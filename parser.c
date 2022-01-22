/* Copyright 2021 Morisaki, Keita. All rights reserved. */
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "./jcc.h"

Token *token;       // token currently processed
char *user_input;   // whole program
Node *statements[100];
LVar *locals_linked_list_head;

void ExitWithErrorAt(char *input, char *loc, char *fmt, ...);
bool StartsWith(char *p, char *suffix);
bool IsAlnumOrUnderscore(char c);

/*** tokenizer ***/
Token *ConnectAndGetNewToken(
    TokenKind kind, Token *current, char *str, int len
  ) {
  Token *new_token = calloc(1, sizeof(Token));
  new_token->kind = kind;
  new_token->str = str;
  new_token->len = len;

  current->next = new_token;
  return new_token;
}

void Tokenize() {
  char *char_pointer = user_input;
  Token head;
  head.next = NULL;
  Token *cur = &head;

  while (*char_pointer) {
    if (isspace(*char_pointer)) {
      ++char_pointer;
      continue;
    }

    if (StartsWith(char_pointer, "return") &&
      !IsAlnumOrUnderscore(char_pointer[6])) {
      cur = ConnectAndGetNewToken(TK_RETURN, cur, char_pointer, 6);
      char_pointer += 6;
      continue;
    }

    if (StartsWith(char_pointer, "if") &&
      !IsAlnumOrUnderscore(char_pointer[2])) {
      cur = ConnectAndGetNewToken(TK_IF, cur, char_pointer, 2);
      char_pointer += 2;
      continue;
    }

    if (StartsWith(char_pointer, "else") &&
      !IsAlnumOrUnderscore(char_pointer[4])) {
      cur = ConnectAndGetNewToken(TK_ELSE, cur, char_pointer, 4);
      char_pointer += 4;
      continue;
    }

    if (StartsWith(char_pointer, "while") &&
      !IsAlnumOrUnderscore(char_pointer[5])) {
      cur = ConnectAndGetNewToken(TK_WHILE, cur, char_pointer, 5);
      char_pointer += 5;
      continue;
    }

    if (StartsWith(char_pointer, "for") &&
      !IsAlnumOrUnderscore(char_pointer[3])) {
      cur = ConnectAndGetNewToken(TK_FOR, cur, char_pointer, 3);
      char_pointer += 3;
      continue;
    }

    if (StartsWith(char_pointer, "==") ||
      StartsWith(char_pointer, "!=") ||
      StartsWith(char_pointer, "<=") ||
      StartsWith(char_pointer, ">=")) {
      cur = ConnectAndGetNewToken(TK_RESERVED, cur, char_pointer, 2);
      char_pointer += 2;
      continue;
    }

    if (('a' <= *char_pointer && *char_pointer <= 'z') ||
      ('A' <= *char_pointer && *char_pointer <= 'Z')) {
      char *start_at = char_pointer;
      while (IsAlnumOrUnderscore(*char_pointer)) {
        ++char_pointer;
      }
      cur = ConnectAndGetNewToken(
        TK_IDENT, cur, start_at, char_pointer-start_at);
      continue;
    }

    if (strchr(";=+-*/()><{}", *char_pointer)) {
      cur = ConnectAndGetNewToken(TK_RESERVED, cur, char_pointer++, 1);
      continue;
    }

    if (isdigit(*char_pointer)) {
      // len is temporarily set to 0
      cur = ConnectAndGetNewToken(TK_NUM, cur, char_pointer, 0);

      char *num_start = char_pointer;
      cur->val = strtol(char_pointer, &char_pointer, 10);
      cur->len = char_pointer - num_start;
      continue;
    }

    ExitWithErrorAt(user_input, char_pointer, "Invalid token.");
  }

  ConnectAndGetNewToken(TK_EOF, cur, char_pointer, 1);
  token = head.next;
}
/*** tokenizer ***/


/*** token processor ***/
bool ConsumeIfReservedTokenMatches(char *op) {
  if (token->kind != TK_RESERVED) {
    return false;
  }
  if (token->len != strlen(op)) {
    return false;
  }
  if (!StartsWith(token->str, op)) {
    return false;
  }

  token = token->next;
  return true;
}

// Consume a token only if the token is TK_IDENT.
// Return the consumed identifier-token, but not the next generated token.
Token *ConsumeAndGetIfIdent() {
  if (token->kind != TK_IDENT) {
    return NULL;
  }

  Token *ident_token = token;
  token = token->next;
  return ident_token;
}

bool ConsumeIfKindMatches(TokenKind kind) {
  if (token->kind != kind) {
    return false;
  }

  token = token->next;
  return true;
}

void Expect(char *op) {
  if (ConsumeIfReservedTokenMatches(op)) return;

  ExitWithErrorAt(user_input, token->str, "Expected `%c`.", *op);
}

int ExpectNumber() {
  if (token->kind != TK_NUM) {
    ExitWithErrorAt(user_input, token->str, "Expected a number.");
  }

  int val = token->val;
  token = token->next;
  return val;
}

bool AtEOF() {
  return token->kind == TK_EOF;
}
/*** token processor ***/


/*** local variable ***/
LVar *GetDeclaredLocal(Token *tok) {
  for (LVar *local = locals_linked_list_head; local; local = local->next) {
    if (local->len != tok->len) continue;
    if (memcmp(local->name, tok->str, local->len)) continue;
    return local;
  }
  return NULL;
}

LVar *NewLVar(Token *tok) {
  LVar *local = calloc(1, sizeof(LVar));
  local->name = tok->str;
  local->len = tok->len;
  if (locals_linked_list_head) {
    local->next = locals_linked_list_head;
    local->offset = locals_linked_list_head->offset + 8;
  } else {
    local->offset = 8;
  }
  // TODO(k1832): exit when number of local variables exceeds the limit.
  locals_linked_list_head = local;
  return local;
}
/*** local variable ***/


/*** AST parser ***/
Node *NewNode(NodeKind kind) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  return node;
}

Node *NewBinary(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = NewNode(kind);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *NewNodeNumber(int val) {
  Node *node = NewNode(ND_NUM);
  node->val = val;
  return node;
}

void Program();
Node *Statement();
Node *Expression();
Node *Assignment();
Node *Equality();
Node *Relational();
Node *Add();
Node *MulDiv();
Node *Unary();
Node *Primary();

// Program    = Statement*
void Program() {
  int i = 0;
  while (!AtEOF()) {
    statements[i++] = Statement();
  }
  statements[i] = NULL;
}

// Statement =
//  "return" Expression ";" |
//  "if" "(" Expression ")" Statement ("else" Statement)? |
//  "while" "(" Expression ")" Statement
//  "for" "(" Expression? ";" Expression? ";" Expression? ")" Statement |
//  "{" Statement* "}" |
//  Expression ";"

Node *Statement() {
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
    if_node->body_statement = Statement();

    if (ConsumeIfKindMatches(TK_ELSE)) {
      if_node->else_statement = Statement();
    }

    return if_node;
  }

  if (ConsumeIfKindMatches(TK_WHILE)) {
    Expect("(");
    Node *lhs = Expression();
    Expect(")");
    return NewBinary(ND_WHILE, lhs, Statement());
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
    for_node->iteration = Expression();
    Expect(")");

    for_node->body_statement = Statement();
    return for_node;
  }

  if (ConsumeIfReservedTokenMatches("{")) {
    Node *nd_block =  NewNode(ND_BLOCK);
    Node *head = nd_block;
    while (!ConsumeIfReservedTokenMatches("}")) {
      nd_block->next_in_block = Statement();
      nd_block = nd_block->next_in_block;
    }
    return head;
  }

  Node *node = Expression();
  Expect(";");
  return node;
}

// Expression       = Assignment
Node *Expression() {
  return Assignment();
}

// Assignment     = Equality ("=" Assignment)?
Node *Assignment() {
  Node *node = Equality();
  if (ConsumeIfReservedTokenMatches("=")) {
    return NewBinary(ND_ASSIGN, node, Assignment());
  }
  return node;
}

// Equality   = Relational ("==" Relational | "!=" Relational)*
Node *Equality() {
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
Node *Relational() {
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
Node *Add() {
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

// MulDiv     = Unary ("*" Unary | "/" Unary)*
Node *MulDiv() {
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

    return node;
  }
}

// Unary   = ("+" | "-")? Primary
Node *Unary() {
  if (ConsumeIfReservedTokenMatches("+")) {
    return Primary();
  }

  if (ConsumeIfReservedTokenMatches("-")) {
    return NewBinary(ND_SUB, NewNodeNumber(0), Primary());
  }

  return Primary();
}

// Primary    = number | identifier | "(" Expression ")"
Node *Primary() {
  if (ConsumeIfReservedTokenMatches("(")) {
    Node *node = Expression();
    Expect(")");
    return node;
  }

  Token *tok = ConsumeAndGetIfIdent();
  if (tok) {
    // identifier
    Node *node = NewNode(ND_LVAR);
    LVar *local = GetDeclaredLocal(tok);
    if (!local) {
      // new variable
      local = NewLVar(tok);
    }
    node->offset = local->offset;
    return node;
  }

  return NewNodeNumber(ExpectNumber());
}
/*** AST parser ***/
