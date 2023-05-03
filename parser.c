/* Copyright 2021 Keita Morisaki. All rights reserved. */
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "./jcc.h"

#define PROGRAM_LEN 100

Node *programs[PROGRAM_LEN];
Node *nd_func_being_defined;

/*** token processor ***/
// DEBUG
// static void DebugToken(char *s) {
//   printf("%s token: %.*s\n", s, token->len, token->str);
// }

static bool ReservedTokenMatches(char *op) {
  return token->kind == TK_RESERVED &&
    token->len == strlen(op) &&
    StartsWith(token->str, op);
}

static void ConsumeToken() {
  token = token->next;
}

static bool ConsumeIfReservedTokenMatches(char *op) {
  if (!ReservedTokenMatches(op)) {
    return false;
  }

  ConsumeToken();
  return true;
}

/*
 * Consume a token only if the token is TK_IDENT.
 * Return the consumed identifier-token, but not the next generated token.
 */
static Token *ConsumeAndGetIfIdent() {
  if (token->kind != TK_IDENT) {
    return NULL;
  }

  Token *ident_token = token;
  ConsumeToken();
  return ident_token;
}

static bool ConsumeIfKindMatches(TokenKind kind) {
  if (token->kind != kind) {
    return false;
  }

  ConsumeToken();
  return true;
}

/*
 * [ignored "-Wreturn-type"]:
 *  This program exits in the error function.
 *  No return value is needed after the function.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
static Token *ExpectIdentifier() {
  Token *tok = ConsumeAndGetIfIdent();
  if (tok) return tok;

  ExitWithErrorAt(user_input, token->str, "Expected identifier.");
}
#pragma GCC diagnostic pop

static void Expect(char *op) {
  if (ConsumeIfReservedTokenMatches(op)) return;

  ExitWithErrorAt(user_input, token->str, "Expected `%c`.", *op);
}

static bool IsNextTokenNumber() {
  return token->kind == TK_NUM;
}

static int ExpectNumber() {
  if (!IsNextTokenNumber()) {
    ExitWithErrorAt(user_input, token->str, "Expected a number.");
  }

  int val = token->val;
  token = token->next;
  return val;
}
/*** token processor ***/


static bool AtEOF() {
  return token->kind == TK_EOF;
}

static void ValidateTypeToken() {
  if (IsTypeToken()) return;

  ExitWithErrorAt(user_input, token->str, "Expected a type token.");
}


static Node *NewNode(NodeKind kind) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  return node;
}

static Node *NewNodeNumber(int val) {
  Node *node = NewNode(ND_NUM);
  node->val = val;
  return node;
}

static Node *NewBinary(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = NewNode(kind);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

static Node *NewUnary(NodeKind kind, Node *nd) {
  Node *node = NewNode(kind);
  node->lhs = nd;
  return node;
}


/*** local variable ***/
static Node *GetDeclaredLocal(Node *nd_block, Token *tok) {
  for (
    Node *local = nd_block->local_var_next;
    local;
    local = local->local_var_next
  ) {
    if (local->var_name_len != tok->len) continue;
    if (strncmp(local->var_name, tok->str, local->var_name_len)) continue;
    return local;
  }
  return NULL;
}

/*
 * Declares new local variable. `array_size` is used
 * only when the `type` is `TY_ARRAY`
 */
static Node *NewLVal(Node *nd_func,
                     Token *ident,
                     Type *type,
                     size_t array_size) {
  Node *lval = NewNode(ND_LVAR_DCLR);

  /*
   * When ident is NULL, it's a temporary variable
   * and ident->str and ident->len will be useless
   */
  if (ident) {
    lval->var_name = ident->str;
    lval->var_name_len = ident->len;
  }

  lval->type = type;
  if (lval->type->kind == TY_ARRAY) {
    lval->type->array_size = array_size;
  }

  if (!nd_func->local_var_next) {
    // First variable
    nd_func->next_offset_in_block = 8;
  }
  // TODO(k1832): exit when number of local variables exceeds the limit.
  lval->local_var_next = nd_func->local_var_next;
  lval->offset = nd_func->next_offset_in_block;

  /*
   * For an array type variable,
   * the variable itself stores the address of the first value
   * of the array.
   * For example:
   *   "int a[3]; a[0]=-1; a[1]=-1;"
   * Memory for the example will be placed like below.
   * addr:   0 1   2
   * value: [0 -1, -1]
   */
  // TODO(k1832): Use GetSize for calculating offset?
  nd_func->next_offset_in_block += 8;   // Memory for the variable
  if (type->kind == TY_ARRAY) {
    // Offset for array elements
    nd_func->next_offset_in_block += 8 * array_size;
  }

  nd_func->local_var_next = lval;
  return lval;
}

/*
 * Gets lval node from identifier and set offset for it,
 * then returns the node
 */
static Node *GetLValNodeFromIdent(Token *ident) {
  Node *lval = NewNode(ND_LVAR);
  Node *local = GetDeclaredLocal(nd_func_being_defined, ident);
  if (!local) {
    return NULL;
  }

  lval->var_name_len = local->var_name_len;
  lval->var_name = local->var_name;

  lval->offset = local->offset;
  lval->type = local->type;
  return lval;
}
/*** local variable ***/


/*** function call/definition ***/
/*
 * Can be called with ND_FUNC_CALL or ND_FUNC_DEFINITION.
 * This function compares 2 nodes and return true iff
 * 2 nodes have the same function name.
 */
static bool FuncNamesMatch(Node *nd_a, Node *nd_b) {
  assert(nd_a && nd_b);
  assert(nd_a->kind == ND_FUNC_CALL || nd_a->kind == ND_FUNC_DEFINITION);
  assert(nd_b->kind == ND_FUNC_CALL || nd_b->kind == ND_FUNC_DEFINITION);

  if (nd_a->func_name_len != nd_b->func_name_len) return false;
  return !strncmp(nd_a->func_name, nd_b->func_name, nd_a->func_name_len);
}

// Make sure one parameter name is used only once at most.
static void ValidateParamName(Node *nd_func, Token *new_param) {
  assert(nd_func->kind == ND_FUNC_DEFINITION);
  assert(new_param->kind == TK_IDENT);

  Node *param = nd_func->param_next;
  while (param) {
    if (new_param->len != param->var_name_len) {
      param = param->param_next;
      continue;
    }

    if (memcmp(new_param->str, param->var_name, new_param->len)) {
      param = param->param_next;
      continue;
    }

    ExitWithErrorAt(user_input, new_param->str,
      "This parameter name is used more than once: \"%.*s\"",
      new_param->len, new_param->str);
  }
}

/*
 * Add a new parameter to a function.
 * Create a new local variable and
 * link it to the parameter linked-list.
 * This linked-list is used to stack
 * actual arguments to the stack frame
 * when the function is called.
 */
static void NewFuncParam(Node *nd_func, Token *ident, Type *type) {
  assert(nd_func->kind == ND_FUNC_DEFINITION);

  ++(nd_func->argc);
  ++(nd_func->num_parameters);
  Node *local = NewLVal(nd_func, ident, type, 0);

  if (nd_func->argc > 6) {
    /*
     * According to the ABI,
     * arguments after the first 6 arguments
     * should be pushed to stack reversely
     * before calling a function.
     * So offsets for them become negative values.
     */
    local->offset = -(8 * (nd_func->argc - 7) + 16);
    nd_func->next_offset_in_block -= 8;
  }

  // Link new variable to linked-list.
  if (nd_func->param_next) {
    local->param_next = nd_func->param_next;
  }
  nd_func->param_next = local;
}

static void NewArg(Node *nd_func_call, Node *new_arg) {
  // Add new argument at the head of linked list
  new_arg->arg_next = nd_func_call->arg_next;
  nd_func_call->arg_next = new_arg;
}
/*** function call/definition ***/


/*
 * Parses type and returns it.
 * Note: Returned type's memory is allocated in heap memory
 * [ignored "-Wreturn-type"]:
 *  This function exits when the token is unexpected type.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
Type *GetType() {
  assert(IsTypeToken());

  // TK_INT,
  Token *base_type_token = token;
  ConsumeToken();  // Skip base type token for now

  Type *type = calloc(1, sizeof(Type));
  Type *current = type;
  while (ConsumeIfReservedTokenMatches("*")) {
    Type *point_to = calloc(1, sizeof(Type));
    current->kind = TY_PTR;
    current->point_to = point_to;
    current = point_to;
  }

  switch (base_type_token->kind) {
  case TK_INT:
    current->kind = TY_INT;
    return type;

  default:
    ExitWithErrorAt(user_input, base_type_token->str, "Unsupported type.");
  }
}
#pragma GCC diagnostic pop

void BuildAST();
static Node *StatementOrExpr();
static Node *Expression();
static Node *Assignment();
static Node *Equality();
static Node *Relational();
static Node *Add();
static Node *MulDiv();
static Node *Unary();
static Node *Dereferenceable();
static Node *LVal();
static Node *Primary();

// Store StatementOrExpr into programs
void BuildAST() {
  int i = 0;
  while (!AtEOF()) {
    if (i >= PROGRAM_LEN - 1)
      ExitWithErrorAt(user_input, token->str, "Exceeds max length of program.");
    programs[i++] = StatementOrExpr();
  }
  programs[i] = NULL;
}

/*
 * TODO(k1832): Declaration of multiple variables
 * TODO(k1832): Accept Expression as array size
 *
 * StatementOrExpr =
 *  "return" Expression ";" |
 *  "if" "(" Expression ")" StatementOrExpr ("else" StatementOrExpr)? |
 *  "while" "(" Expression ")" StatementOrExpr
 *  "for" "(" Expression? ";" Expression? ";" Expression? ")" StatementOrExpr |
 *  "{" StatementOrExpr* "}" |
 *  Expression ";" |
 *  "int" "*"* identifier ("[" number "]")? ";" |
 *
 *  "int" "*"* identifier "(" ( "int" "*"* identifier ("," "int" "*"* identifier )? ")" "{"
 *    StatementOrExpr*
 *  "}"
 *
 */
static Node *StatementOrExpr() {
  if (ConsumeIfKindMatches(TK_RETURN)) {
    Node *lhs = Expression();
    Expect(";");
    return NewUnary(ND_RETURN, lhs);
  }

  if (ConsumeIfKindMatches(TK_IF)) {
    Node *if_node = NewNode(ND_IF);
    Expect("(");
    if_node->condition = Expression();
    Expect(")");
    if_node->body_statement = StatementOrExpr();

    if (ConsumeIfKindMatches(TK_ELSE)) {
      if_node->else_statement = StatementOrExpr();
    }

    return if_node;
  }

  if (ConsumeIfKindMatches(TK_WHILE)) {
    Expect("(");
    Node *lhs = Expression();
    Expect(")");
    return NewBinary(ND_WHILE, lhs, StatementOrExpr());
  }

  if (ConsumeIfKindMatches(TK_FOR)) {
    Node *for_node = NewNode(ND_FOR);

    Expect("(");
    if (!ConsumeIfReservedTokenMatches(";")) {
      for_node->initialization = Expression();
      Expect(";");
    }
    if (!ConsumeIfReservedTokenMatches(";")) {
      for_node->condition = Expression();
      Expect(";");
    }
    if (!ReservedTokenMatches(")")) {
      for_node->iteration = Expression();
    }
    Expect(")");

    for_node->body_statement = StatementOrExpr();
    return for_node;
  }


  //  "{" StatementOrExpr* "}"
  if (ConsumeIfReservedTokenMatches("{")) {
    Node *nd_block =  NewNode(ND_BLOCK);
    Node *head = nd_block;
    while (!ConsumeIfReservedTokenMatches("}")) {
      nd_block->next_in_block = StatementOrExpr();
      nd_block = nd_block->next_in_block;
    }
    return head;
  }

  if (!IsTypeToken()) {
    //  Expression ";"
    Node *node = Expression();
    Expect(";");
    return node;
  }

  // "int" "*"* identifier ("[" number "]")? ";"
  Type *ret_or_var_type = GetType();

  Token *variable_or_func_name = ExpectIdentifier();
  if (ConsumeIfReservedTokenMatches("[")) {
    size_t array_size = (size_t)ExpectNumber();

    // local variable
    Node *local =
      GetDeclaredLocal(nd_func_being_defined, variable_or_func_name);

    if (local) {
      ExitWithErrorAt(user_input, variable_or_func_name->str,
        "Redeclaration of \"%.*s\"",
        variable_or_func_name->len,
        variable_or_func_name->str);
    }
    Type *curr_ty = ret_or_var_type;

    // NOLINTNEXTLINE(readability/braces)
    // Type *array_type = &(Type){TY_ARRAY, curr_ty};

    Type *array_type = calloc(1, sizeof(Type));
    array_type->kind = TY_ARRAY;
    array_type->point_to = curr_ty;

    Node *node = NewLVal(nd_func_being_defined,
                         variable_or_func_name,
                         array_type, array_size);
    Expect("]");
    Expect(";");
    return node;
  }

  if (ConsumeIfReservedTokenMatches(";")) {
    // local variable
    Node *local =
      GetDeclaredLocal(nd_func_being_defined, variable_or_func_name);

    if (local) {
      ExitWithErrorAt(user_input, variable_or_func_name->str,
        "Redeclaration of \"%.*s\"",
        variable_or_func_name->len,
        variable_or_func_name->str);
    }

    return NewLVal(nd_func_being_defined,
                   variable_or_func_name,
                   ret_or_var_type, 0);
  }

  /*
   * "int" "*"* identifier "(" ( "int" "*"* identifier ("," "int" "*"* identifier) )? ")" "{"
   *   StatementOrExpr*
   * "}"
   *
   * Function declaration
   * TODO(k1832): Check for multiple definition with same name
   */
  Node *nd_func_define = NewNode(ND_FUNC_DEFINITION);
  nd_func_define->func_name = variable_or_func_name->str;
  nd_func_define->func_name_len = variable_or_func_name->len;
  nd_func_define->ret_type = ret_or_var_type;

  Expect("(");

  while (IsTypeToken()) {
    Type *param_type = GetType();
    Token *ident_param = ExpectIdentifier();
    ValidateParamName(nd_func_define, ident_param);
    NewFuncParam(nd_func_define, ident_param, param_type);

    if (!ConsumeIfReservedTokenMatches(",")) {
      break;
    }

    ValidateTypeToken();
  }

  Expect(")");

  Expect("{");
  nd_func_being_defined = nd_func_define;

  Node *node_in_block = nd_func_define;
  while (!ConsumeIfReservedTokenMatches("}")) {
    node_in_block->next_in_block = StatementOrExpr();
    node_in_block = node_in_block->next_in_block;
  }
  // Reset the node that's currently being processed function.
  nd_func_being_defined = NULL;

  return nd_func_define;
}

// Expression     = Assignment
static Node *Expression() {
  return Assignment();
}

// Convert `lhs op= rhs` to `tmp = &lhs, *tmp = *tmp op rhs`
static Node *ToAssign(NodeKind op_type, Node *lhs, Node *rhs) {
  AddType(lhs);
  AddType(rhs);

  Type *ty_pointer_to_lhs = calloc(1, sizeof(Type));
  ty_pointer_to_lhs->kind = TY_PTR;
  ty_pointer_to_lhs->point_to = lhs->type;

  // tmp
  Node *tmp = NewLVal(nd_func_being_defined,
                                 NULL, ty_pointer_to_lhs, 0);

  /*
   * `tmp->kind` is `ND_LVAR_DCLR` here.
   * For other use cases of `ND_LVAR_DCLR`,
   * every time the variable is used, a new node
   * is created as the `ND_LVAR` type
   * by calling `GetLValNodeFromIdent`.
   * However, temporary variable does not have its name
   * and the node cannot be made in the same way.
   * Also, since the temporary variable is used only once,
   * the `tmp` node is explicitly changed to the `ND_LVAR` node
   * here.
   */
  tmp->kind = ND_LVAR;

  // tmp = &lhs
  Node *expr1 = NewBinary(ND_ASSIGN, tmp, NewUnary(ND_ADDR, lhs));

  // *tmp = *tmp op rhs
  Node *expr2 =
    NewBinary(ND_ASSIGN,
               NewUnary(ND_DEREF, tmp),
               NewBinary(op_type,
                          NewUnary(ND_DEREF, tmp),
                          rhs));


  return NewBinary(ND_COMMA, expr1, expr2);
}

/*
 * Assignment =
 *  Equality "=" Assignment |
 *  Equality "+=" Assignment |
 *  Equality "-=" Assignment |
 *  Equality "*=" Assignment |
 *  Equality "/=" Assignment |
 *  Equality "%=" Assignment |
 *  Equality
 */
static Node *Assignment() {
  Node *equality = Equality();
  if (ConsumeIfReservedTokenMatches("="))
    return NewBinary(ND_ASSIGN, equality, Assignment());

  if (ConsumeIfReservedTokenMatches("+="))
    return ToAssign(ND_ADD, equality, Assignment());

  if (ConsumeIfReservedTokenMatches("-="))
    return ToAssign(ND_SUB, equality, Assignment());

  if (ConsumeIfReservedTokenMatches("*="))
    return ToAssign(ND_MUL, equality, Assignment());

  if (ConsumeIfReservedTokenMatches("/="))
    return ToAssign(ND_DIV, equality, Assignment());

  if (ConsumeIfReservedTokenMatches("%="))
    return ToAssign(ND_MOD, equality, Assignment());

  return equality;
}

// Equality   = Relational ("==" Relational | "!=" Relational)*
static Node *Equality() {
    Node *node = Relational();
    for (;;) {
      if (ConsumeIfReservedTokenMatches("==")) {
        node = NewBinary(ND_EQ, node, Relational());
        continue;
      }

      if (ConsumeIfReservedTokenMatches("!=")) {
        node = NewBinary(ND_NEQ, node, Relational());
        continue;
      }

      return node;
    }
}

// Relational = Add ("<" Add | "<=" Add | ">" Add | ">=" Add)*
static Node *Relational() {
  Node *node = Add();
  for (;;) {
    if (ConsumeIfReservedTokenMatches("<")) {
      node = NewBinary(ND_LT, node, Add());
      continue;
    }

    if (ConsumeIfReservedTokenMatches(">")) {
      // (Add() < node) == (node > Add())
      node = NewBinary(ND_LT, Add(), node);
      continue;
    }

    if (ConsumeIfReservedTokenMatches("<=")) {
      node = NewBinary(ND_NGT, node, Add());
      continue;
    }

    if (ConsumeIfReservedTokenMatches(">=")) {
      // (Add() <= node) == (node >= Add())
      node = NewBinary(ND_NGT, Add(), node);
      continue;
    }

    return node;
  }
}

// Return true iff the type is pointer or array
static bool IsPointerLike(Type *ty) {
  if (ty->kind == TY_PTR) return true;
  return ty->kind == TY_ARRAY;
}

static Node *NewAdd(Node *lhs, Node *rhs) {
  AddType(lhs);
  AddType(rhs);

  // Implicit type conversion

  // num + num
  if (lhs->type->kind == TY_INT && rhs->type->kind == TY_INT) {
    return NewBinary(ND_ADD, lhs, rhs);
  }

  if (IsPointerLike(lhs->type) && IsPointerLike(rhs->type)) {
    // TODO(k1832): Add "Token" to each "Node" for better error message
    ExitWithError("Invalid operands.");
  }

  // "num + ptr" -> "ptr + num"
  if (IsPointerLike(rhs->type)) {
    // swap "lhs" and "rhs"
    Node *tmp = lhs;
    lhs = rhs;
    rhs = tmp;
  }

  // (ptr + num) -> ptr - (sizeof(*ptr) * num)
  // TODO(k1832): Replace "8" with sizeof(*ptr)
  rhs = NewBinary(ND_MUL, rhs, NewNodeNumber(8));
  return NewBinary(ND_SUB, lhs, rhs);
}

/*
 * Subtraction between 2 numbers, 1 number and 1 pointer,
 * or 2 pointers.
 *
 * [ignored "-Wreturn-type"]:
 *  This program exits in the error function.
 *  No return value is needed after the function.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
static Node *NewSub(Node *lhs, Node *rhs) {
  AddType(lhs);
  AddType(rhs);

  // Implicit type conversion

  // num - num
  if (lhs->type->kind == TY_INT && rhs->type->kind == TY_INT) {
    return NewBinary(ND_SUB, lhs, rhs);
  }

  // ptr - num
  if (IsPointerLike(lhs->type) && rhs->type->kind == TY_INT) {
    // "ptr - num" -> "ptr + sizeof(*ptr) * num"
    rhs = NewBinary(ND_MUL, rhs, NewNodeNumber(8));
    Node *node = NewBinary(ND_ADD, lhs, rhs);
    node->type = lhs->type;
    return node;
  }

  // ptr - ptr
  if (lhs->type->kind == TY_PTR && rhs->type->kind == TY_PTR) {
    /*
    * ptr - ptr will return the distance between 2 elements
    *
    * int a[10];
    * int *b = &(a[0]);
    * int *c = &(a[2]);
    * printf("%ld\n", c - b);
    *
    * -> This will print "2".
    */

    /*
     * In x86-64 architecture, the stack grows downwards
     * (i.e., from higher addresses to lower addresses).
     * When calculating the difference between two pointers,
     * we reverse the operands (rhs - lhs) to ensure that the resulting
     * value is positive and correctly represents the number of elements
     * between the pointers.
     */
    Node *node = NewBinary(ND_SUB, rhs, lhs);
    node->type = ty_int;
    return NewBinary(ND_DIV, node, NewNodeNumber(8));
  }

  // TODO(k1832): Add "Token" to each "Node" for better error message
  ExitWithError("Invalid operands.");
}
#pragma GCC diagnostic pop

// Add    = MulDiv ("+" MulDiv | "-" MulDiv)*
static Node *Add() {
  Node *node = MulDiv();
  for (;;) {
    if (ConsumeIfReservedTokenMatches("+")) {
      node = NewAdd(node, MulDiv());
      continue;
    }

    if (ConsumeIfReservedTokenMatches("-")) {
      node = NewSub(node, MulDiv());
      continue;
    }

    return node;
  }
}

// MulDiv     = Unary ("*" Unary | "/" Unary | "%" Unary)*
static Node *MulDiv() {
  Node *node = Unary();
  for (;;) {
    if (ConsumeIfReservedTokenMatches("*")) {
      node = NewBinary(ND_MUL, node, Unary());
      continue;
    }

    if (ConsumeIfReservedTokenMatches("/")) {
      node = NewBinary(ND_DIV, node, Unary());
      continue;
    }

    if (ConsumeIfReservedTokenMatches("%")) {
      node = NewBinary(ND_MOD, node, Unary());
      continue;
    }

    return node;
  }
}

/*
 * Unary   =
 *  "sizeof" Unary |
 *  ("+" | "-") Primary |
 *  "*" Dereferenceable |
 *  "&" LVal |
 *  ("++" | "--") LVal |
 *  LVal ("++" | "--")? |
 *  Primary
 */
static Node *Unary() {
  if (ConsumeIfReservedTokenMatches("sizeof")) {
    Node *node = Unary();
    AddType(node);
    return NewNodeNumber(GetSize(node->type));
  }

  if (ConsumeIfReservedTokenMatches("+")) {
    // Just remove "+"
    return Expression();
  }

  if (ConsumeIfReservedTokenMatches("-")) {
    // Replace with "0 - Node"
    return NewBinary(ND_SUB, NewNodeNumber(0), Primary());
  }

  if (ConsumeIfReservedTokenMatches("++")) {
    // "++i" -> "i += 1"
    return ToAssign(ND_ADD, LVal(), NewNodeNumber(1));
  }

  if (ConsumeIfReservedTokenMatches("--")) {
    // "--i" -> "i -= 1"
    return ToAssign(ND_SUB, LVal(), NewNodeNumber(1));
  }

  if (ConsumeIfReservedTokenMatches("*")) {
    return NewUnary(ND_DEREF, Dereferenceable());
  }

  // "&" LVal
  if (ConsumeIfReservedTokenMatches("&")) {
    return NewUnary(ND_ADDR, LVal());
  }

  Node *lval = LVal();

  if (!lval) {
    return Primary();
  }

  // LVal ("++" | "--") |
  if (ConsumeIfReservedTokenMatches("++")) {
    // "i++" -> "(i+=1) - 1"
    return NewBinary(ND_SUB,
                      ToAssign(ND_ADD, lval, NewNodeNumber(1)),
                      NewNodeNumber(1));
  }
  if (ConsumeIfReservedTokenMatches("--")) {
    // "i--" -> "(i-=1) + 1"
    return NewBinary(ND_ADD,
                      ToAssign(ND_SUB, lval, NewNodeNumber(1)),
                      NewNodeNumber(1));
  }

  return lval;
}

/*
 * Dereferenceable =
 *  "*" Dereferenceable |
 *  "&" Lval |
 *  "(" Expression ")" |
 *  Lval
 */
static Node *Dereferenceable() {
  // "*" Dereferenceable
  if (ConsumeIfReservedTokenMatches("*")) {
    return NewUnary(ND_DEREF, Dereferenceable());
  }

  // "&" identifier
  if (ConsumeIfReservedTokenMatches("&")) {
    Node *lval = LVal();
    if (!lval) {
      ExitWithErrorAt(user_input, token->str, "Undeclared variable");
    }
    return NewUnary(ND_ADDR, lval);
  }

  if (ConsumeIfReservedTokenMatches("(")) {
    Node *expression = Expression();
    Expect(")");
    return expression;
  }

  Node *lval = LVal();
  if (!lval) {
    ExitWithErrorAt(user_input, token->str, "Undeclared variable");
  }

  return lval;
}

/*
 * Parses tokens.
 * Returns LVal node on success, otherwise returns NULL.
 * Before returning NULL, the token is restored to its state
 * at the time the function was called.
 *
 * LVal =
 *  "*" Dereferenceable |
 *  identifier ("[" Expression "]")?
 */
static Node *LVal() {
  Token *stashed_token = token;

  // "*" Dereferenceable |
  if (ConsumeIfReservedTokenMatches("*")) {
    return NewUnary(ND_DEREF, Dereferenceable());
  }

  Token *ident = ConsumeAndGetIfIdent();
  if (ident) {
    Node *nd_lval = GetLValNodeFromIdent(ident);
    if (nd_lval) {
      if (ConsumeIfReservedTokenMatches("[")) {
        Node *expression = Expression();
        Expect("]");

        // ptr[index] -> *(ptr + index)
        return NewUnary(ND_DEREF, NewAdd(nd_lval, expression));
      }
      return nd_lval;
    }
  }

  token = stashed_token;
  return NULL;
}

/*
 * TODO(k1832): How to write comma-separated arguments for a function in EBNF?
 *
 * Primary =
 *  "(" Expression ")" |
 *  identifier "(" ( Expression ("," Expression)* )? ")" ) |
 *  LVal
 *  number
 */
static Node *Primary() {
  if (ConsumeIfReservedTokenMatches("(")) {
    Node *node = Expression();
    Expect(")");
    return node;
  }

  Token *stashed_token = token;
  Token *tok = ConsumeAndGetIfIdent();
  if (tok) {
    // identifier
    if (ConsumeIfReservedTokenMatches("(")) {
      // Function call

      Node *nd_func_call = NewNode(ND_FUNC_CALL);
      nd_func_call->func_name = tok->str;
      nd_func_call->func_name_len = tok->len;
      while (!ConsumeIfReservedTokenMatches(")")) {
        ++(nd_func_call->argc);
        NewArg(nd_func_call, Expression());
        ConsumeIfReservedTokenMatches(",");
        /*
         * TODO(k1832): Consider a behavior
         * when there is no argument after a comma.
         */
      }

      // Find the corresponding function definition
      for (int i = 0; i < 100; ++i) {
        Node *nd = programs[i];
        if (!nd)
          break;
        if (nd->kind != ND_FUNC_DEFINITION)
          continue;
        if (!FuncNamesMatch(nd, nd_func_call))
          continue;

        nd_func_call->func_def = nd;
        return nd_func_call;
      }

      // Also function that's currenly being declared is called (recursion)
      if (FuncNamesMatch(nd_func_being_defined, nd_func_call)) {
        nd_func_call->func_def = nd_func_being_defined;
        return nd_func_call;
      }
    }
    token = stashed_token;
  }

  Node *node = LVal();
  if (node) return node;

  if (IsNextTokenNumber()) {
    return NewNodeNumber(ExpectNumber());
  }

  /*
   * "token" should be restored such that
   * "token" is same as the token before this function is called.
   */
  return NULL;
}
