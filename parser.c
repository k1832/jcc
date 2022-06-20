/* Copyright 2021 Keita Morisaki. All rights reserved. */
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "./jcc.h"

Node *programs[100];
Node *nd_func_dclr_in_progress;

void ExitWithErrorAt(char *input, char *loc, char *fmt, ...);
bool StartsWith(char *p, char *suffix);

/*** token processor ***/
static bool ReservedTokenMatches(char *op) {
  return token->kind == TK_RESERVED &&
    token->len == strlen(op) &&
    StartsWith(token->str, op);
}

static bool ConsumeIfReservedTokenMatches(char *op) {
  if (!ReservedTokenMatches(op)) {
    return false;
  }

  token = token->next;
  return true;
}

// Consume a token only if the token is TK_IDENT.
// Return the consumed identifier-token, but not the next generated token.
static Token *ConsumeAndGetIfIdent() {
  if (token->kind != TK_IDENT) {
    return NULL;
  }

  Token *ident_token = token;
  token = token->next;
  return ident_token;
}

static bool ConsumeIfKindMatches(TokenKind kind) {
  if (token->kind != kind) {
    return false;
  }

  token = token->next;
  return true;
}

static void ValidateToken(TokenKind kind) {
  if (token->kind == kind) return;

  ExitWithErrorAt(user_input, token->str, "Unexpected token.");
}

static Token *ExpectIdentifier() {
  Token *tok = ConsumeAndGetIfIdent();
  if (tok) return tok;

  ExitWithErrorAt(user_input, token->str, "Expected identifier.");
}

static void ExpectSpecificToken(TokenKind kind) {
  if (ConsumeIfKindMatches(kind)) return;

  ExitWithErrorAt(user_input, token->str, "Unexpected token.");
}

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

static bool AtEOF() {
  return token->kind == TK_EOF;
}
/*** token processor ***/


/*** local variable ***/
static LVar *GetDeclaredLocal(Node *node, Token *tok) {
  for (
    LVar *local = node->locals_linked_list_head;
    local;
    local = local->next
  ) {
    if (local->len != tok->len) continue;
    if (memcmp(local->name, tok->str, local->len)) continue;
    return local;
  }
  return NULL;
}

static LVar *NewLVar(Node *node, Token *tok) {
  LVar *local = calloc(1, sizeof(LVar));
  local->name = tok->str;
  local->len = tok->len;
  if (!node->locals_linked_list_head) {
    node->next_offset_in_block = 8;
  }
  // TODO(k1832): exit when number of local variables exceeds the limit.
  local->next = node->locals_linked_list_head;
  local->offset = node->next_offset_in_block;
  node->next_offset_in_block += 8;
  node->locals_linked_list_head = local;
  return local;
}

// Make sure one parameter name is used only once at most.
static void ValidateParamName(Node *node, Token *new_param) {
  assert(new_param->kind == TK_IDENT);

  LVar *param = node->params_linked_list_head;
  while (param) {
    if (new_param->len != param->len) {
      param = param->next;
      continue;
    }

    if (memcmp(new_param->str, param->name, new_param->len)) {
      param = param->next;
      continue;
    }

    ExitWithErrorAt(user_input, new_param->str,
      "This parameter name is used more than once: \"%.*s\"",
      new_param->len, new_param->str);
  }
}

static void NewParam(Node *node, Token *tok) {
  // Create a new local variable and
  // link it to the linked-list of parameters.
  // This linked list is used for
  // transfering values into stack frame.
  ++(node->argc);
  ++(node->num_parameters);
  LVar *local = NewLVar(node, tok);

  if (node->argc > 6) {
    // According to the ABI,
    // arguments after the first 6 arguments
    // should be pushed to stack reversely
    // before calling a function.
    // So offsets for them become negative values.
    local->offset = -(8 * (node->argc - 7) + 16);
    node->next_offset_in_block -= 8;
  }

  // Link new variable to linked-list.
  if (node->params_linked_list_head) {
    local->next = node->params_linked_list_head;
  }
  node->params_linked_list_head = local;
}
/*** local variable ***/


/*** function call ***/
static ArgsForCall *NewArg(Node *nd_func_call, Node *new_arg) {
  ArgsForCall *arg = calloc(1, sizeof(ArgsForCall));
  arg->node = new_arg;
  arg->next = nd_func_call->args_linked_list_head;
  nd_func_call->args_linked_list_head = arg;
  return arg;
}
/*** function call ***/


/*** AST parser ***/
static Node *NewNode(NodeKind kind) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  return node;
}

static Node *NewBinary(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = NewNode(kind);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

static Node *NewNodeNumber(int val) {
  Node *node = NewNode(ND_NUM);
  node->val = val;
  return node;
}

void BuildAST();
static Node *StatementOrExpr();
static Node *Expression();
static Node *Assignment();
static Node *Equality();
static Node *Relational();
static Node *Add();
static Node *MulDiv();
static Node *Unary();
static Node *RefDefLvar();
static Node *Primary();

// Store StatementOrExpr into programs
void BuildAST() {
  int i = 0;
  while (!AtEOF()) {
    programs[i++] = StatementOrExpr();
  }
  programs[i] = NULL;
}

// TODO(k1832): How to write comma-separated arguments for a function in EBNF?
// TODO(k1832): Declaration of multiple variables

// StatementOrExpr =
//  "return" Expression ";" |
//  "if" "(" Expression ")" StatementOrExpr ("else" StatementOrExpr)? |
//  "while" "(" Expression ")" StatementOrExpr
//  "for" "(" Expression? ";" Expression? ";" Expression? ")" StatementOrExpr |
//  "{" StatementOrExpr* "}" |
//  Expression ";"
//  "int" identifier "(" ("int" identifier)* ")" "{" StatementOrExpr* "}" |
//  "int" "*"? identifier ";" |

static Node *StatementOrExpr() {
  if (ConsumeIfKindMatches(TK_RETURN)) {
    Node *lhs = Expression();
    Expect(";");
    return NewBinary(ND_RETURN, lhs, NULL);
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

  //  Expression ";"
  if (!ConsumeIfKindMatches(TK_INT)) {
    Node *node = Expression();
    Expect(";");
    return node;
  }

  //  "int" "*"? identifier ";"
  // TOOD(k1832): Distinguish int, int pointer, and etc.
  ConsumeIfReservedTokenMatches("*");

  Token *token_after_variable_type = token;

  Token *variable_or_func_name = ExpectIdentifier();
  if (ConsumeIfReservedTokenMatches(";")) {
    // local variable
    Node *node = NewNode(ND_LVAR);
    LVar *local =
      GetDeclaredLocal(nd_func_dclr_in_progress, variable_or_func_name);

    if (local) {
      ExitWithErrorAt(user_input, token_after_variable_type->str,
        "Redeclaration of \"%.*s\"",
        variable_or_func_name->len,
        variable_or_func_name->str);
    }

    local = NewLVar(nd_func_dclr_in_progress, variable_or_func_name);
    node->offset = local->offset;
    return node;
  }

  //  "int" identifier "(" ("int" identifier)* ")" "{" StatementOrExpr* "}"
  // Function declaration
  // TODO(k1832): Check for multiple definition with same name
  Node *nd_func_dclr = NewNode(ND_FUNC_DECLARATION);
  nd_func_dclr->func_name = variable_or_func_name->str;
  nd_func_dclr->func_name_len = variable_or_func_name->len;

  Expect("(");
  while (ConsumeIfKindMatches(TK_INT)) {
    Token *ident_param = ExpectIdentifier();
    ValidateParamName(nd_func_dclr, ident_param);
    NewParam(nd_func_dclr, ident_param);
    if (!ConsumeIfReservedTokenMatches(",")) {
      break;
    }
    ValidateToken(TK_INT);
  }
  Expect(")");

  Expect("{");
  nd_func_dclr_in_progress = nd_func_dclr;

  Node *node_in_block = nd_func_dclr;
  while (!ConsumeIfReservedTokenMatches("}")) {
    node_in_block->next_in_block = StatementOrExpr();
    node_in_block = node_in_block->next_in_block;
  }
  // Reset the node that's currently being processed function.
  nd_func_dclr_in_progress = NULL;

  return nd_func_dclr;
}

// Expression     = Assignment
static Node *Expression() {
  return Assignment();
}

// Assignment     = Equality ("=" Assignment)?
static Node *Assignment() {
  Node *node = Equality();
  if (ConsumeIfReservedTokenMatches("=")) {
    return NewBinary(ND_ASSIGN, node, Assignment());
  }
  return node;
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
      // Add() < node = node > Add()
      node = NewBinary(ND_LT, Add(), node);
      continue;
    }

    if (ConsumeIfReservedTokenMatches("<=")) {
      node = NewBinary(ND_NGT, node, Add());
      continue;
    }

    if (ConsumeIfReservedTokenMatches(">=")) {
      // Add() <= node = node >= Add()
      node = NewBinary(ND_NGT, Add(), node);
      continue;
    }

    return node;
  }
}

// Add    = MulDiv ("+" MulDiv | "-" MulDiv)*
static Node *Add() {
  Node *node = MulDiv();
  for (;;) {
    if (ConsumeIfReservedTokenMatches("+")) {
      node = NewBinary(ND_ADD, node, MulDiv());
      continue;
    }

    if (ConsumeIfReservedTokenMatches("-")) {
      node = NewBinary(ND_SUB, node, MulDiv());
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

// Unary   =
//  ("+" | "-") Primary
//  ("+" | "-") RefDefLvar ("++" | "--")?
//  ("++" | "--") RefDefLvar
//  Primary |
//  RefDefLvar ("++" | "--")?
static Node *Unary() {
  if (ConsumeIfReservedTokenMatches("+")) {
    // Just remove "+"

    Node *primary = Primary();
    if (primary) {
      return primary;
    }

    Node *node = RefDefLvar();
    if (ConsumeIfReservedTokenMatches("++")) {
      node->post_increment = true;
      return node;
    }
    if (ConsumeIfReservedTokenMatches("--")) {
      node->post_decrement = true;
      return node;
    }

    return node;
  }

  if (ConsumeIfReservedTokenMatches("-")) {
    // Replace with "0 - Node"

    Node *primary = Primary();
    if (primary) {
      return NewBinary(ND_SUB, NewNodeNumber(0), primary);
    }

    Node *rhs = RefDefLvar();
    if (ConsumeIfReservedTokenMatches("++")) {
      rhs->post_increment = true;
    }
    if (ConsumeIfReservedTokenMatches("--")) {
      rhs->post_decrement = true;
    }
    Node *node = NewBinary(ND_SUB, NewNodeNumber(0), rhs);

    return node;
  }

  if (ConsumeIfReservedTokenMatches("++")) {
    return NewBinary(ND_PRE_INCREMENT, RefDefLvar(), NULL);
  }

  if (ConsumeIfReservedTokenMatches("--")) {
    return NewBinary(ND_PRE_DECREMENT, RefDefLvar(), NULL);
  }

  Node *node = Primary();
  if (node) return node;

  node = RefDefLvar();
  if (ConsumeIfReservedTokenMatches("++")) {
    node->post_increment = true;
  }
  if (ConsumeIfReservedTokenMatches("--")) {
    node->post_decrement = true;
  }
  return node;
}

// Post increment and decrement could happen with these ND_KIND

// RefDefLvar =
//  ("*" | "&")? RefDefLvar |
//  identifier
static Node *RefDefLvar() {
  if (ConsumeIfReservedTokenMatches("*")) {
    return NewBinary(ND_DEREF, RefDefLvar(), NULL);
  }

  if (ConsumeIfReservedTokenMatches("&")) {
    return NewBinary(ND_ADDR, RefDefLvar(), NULL);
  }

  Token *ident = ExpectIdentifier();
  Node *node = NewNode(ND_LVAR);
  LVar *local = GetDeclaredLocal(nd_func_dclr_in_progress, ident);
  if (!local) {
    ExitWithErrorAt(user_input, ident->str,
      "Undeclared variable \"%.*s\"", ident->len, ident->str);
  }
  node->offset = local->offset;
  return node;
}

// TODO(k1832): How to write comma-separated arguments for a function in EBNF?

// Primary =
//  "(" Expression ")" |
//  identifier "(" Expression* ")" ) |
//  number
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
        // TODO(k1832): Consider a behavior
        // when there is no argument after a comma.
      }
      return nd_func_call;
    }
    token = stashed_token;
  }
  if (IsNextTokenNumber()) {
    return NewNodeNumber(ExpectNumber());
  }

  // "token" should be restored such that
  // "token" is same as the token before this function is called.
  return NULL;
}
/*** AST parser ***/
