/* Copyright 2021 Morisaki, Keita. All rights reserved. */
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "./jcc.h"

Token *token;       // token currently processed
char *user_input;   // whole program

void ExitWithErrorAt(char *input, char *loc, char *fmt, ...);
bool StartsWith(char *p, char *suffix);

/*** tokenizer ***/
Token *ConnectAndGetNewToken(TokenKind kind, Token *cur, char *str, int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  tok->len = len;

  cur->next = tok;
  return tok;
}

Token *Tokenize() {
  char *char_pointer = user_input;
  Token head;
  head.next = NULL;
  Token *cur = &head;

  while (*char_pointer) {
    if (isspace(*char_pointer)) {
      ++char_pointer;
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

    if (strchr("+-*/()><", *char_pointer)) {
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
  return head.next;
}
/*** tokenizer ***/


/*** token processor ***/
bool ConsumeIfReservedTokenMatches(char *op) {
  if (token->kind != TK_RESERVED ||
    token->len != strlen(op) ||
    memcmp(token->str, op, token->len))
    return false;

  token = token->next;
  return true;
}

void Expect(char *op) {
  if (!ConsumeIfReservedTokenMatches(op))
    ExitWithErrorAt(user_input, token->str, "Expected %c.", *op);
}

int ExpectNumber() {
  if (token->kind != TK_NUM)
    ExitWithErrorAt(user_input, token->str, "Expected a number.");

  int val = token->val;
  token = token->next;
  return val;
}

bool AtEOF() {
  return token->kind == TK_EOF;
}
/*** token processor ***/


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

Node *Expression();
Node *Equality();
Node *Relational();
Node *Add();
Node *MulDiv();
Node *Unary();
Node *Primary();

Node *Expression() {
  return Equality();
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
  if (ConsumeIfReservedTokenMatches("+"))
    return Primary();

  if (ConsumeIfReservedTokenMatches("-"))
    return NewBinary(ND_SUB, NewNodeNumber(0), Primary());

  return Primary();
}

// Primary = number | "(" Add ")"
Node *Primary() {
  if (ConsumeIfReservedTokenMatches("(")) {
    Node *node = Expression();
    Expect(")");
    return node;
  }

  return NewNodeNumber(ExpectNumber());
}
/*** AST parser ***/
