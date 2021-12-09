/* Copyright 2021 Morisaki, Keita. All rights reserved. */
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*** Token definition ***/
typedef enum {
  TK_RESERVED,
  TK_NUM,
  TK_EOF,
} TokenKind;

typedef struct Token Token;
struct Token {
  TokenKind kind;
  Token *next;
  int val;
  char *str;
  int len;
};
/*** Token definition ***/


/*** AST definition ***/
typedef enum {
  ND_ADD,
  ND_SUB,
  ND_MUL,
  ND_DIV,
  ND_EQ,
  ND_NEQ,
  ND_LT,
  ND_NGT,
  ND_NUM,
} NodeKind;

typedef struct Node Node;
struct Node {
  NodeKind kind;
  Node *lhs;
  Node *rhs;
  int val;
};
/*** AST definition ***/


/*** GLOBAL VARIALBES ***/
Token *token;       // token currently processed
char *user_input;   // whole program
/*** GLOBAL VARIALBES ***/


/*** error ***/
void ExitWithErrorAt(char *loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  int pos = loc - user_input;
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s", pos, " ");
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

void ExitWithError(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}
/*** error ***/


/*** tokenizer ***/
Token *ConnectAndGetNewToken(TokenKind kind, Token *cur, char *str, int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  tok->len = len;

  cur->next = tok;
  return tok;
}

bool StartsWith(char *p, char *suffix) {
  return !memcmp(p, suffix, strlen(suffix));
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

    ExitWithErrorAt(char_pointer, "Invalid token.");
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
    ExitWithErrorAt(token->str, "Expected %c.", *op);
}

int ExpectNumber() {
  if (token->kind != TK_NUM)
    ExitWithErrorAt(token->str, "Expected a number.");

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


/*** code generator ***/
void PrintAssembly(Node *node) {
  if (node->kind == ND_NUM) {
    printf("  push %d\n", node->val);
    return;
  }

  PrintAssembly(node->lhs);
  PrintAssembly(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->kind) {
    case ND_ADD:
      printf("  add rax, rdi\n");
      break;
    case ND_SUB:
      printf("  sub rax, rdi\n");
      break;
    case ND_MUL:
      printf("  imul rax, rdi\n");
      break;
    case ND_DIV:
      printf("  cqo\n");
      printf("  idiv rdi\n");
      break;
    case ND_EQ:
      printf("  cmp rax, rdi\n");
      printf("  sete al\n");  // al is the bottom 8 bits of rax
      printf("  movzb rax, al\n");  // zero-fill top 56 bits
      break;
    case ND_LT:
      // rax < rdi
      printf("  cmp rax, rdi\n");
      printf("  setl al\n");
      printf("  movzb rax, al\n");
      break;
    case ND_NGT:
      // rax <= rdi
      printf("  cmp rax, rdi\n");
      printf("  setle al\n");
      printf("  movzb rax, al\n");
      break;
  }

  printf("  push rax\n");
}
/*** code generator ***/


int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Exactly one argument must be passed.\n");
    return 1;
  }

  user_input = argv[1];
  token = Tokenize();
  Node *ast_root = Expression();

  printf(".intel_syntax noprefix\n");
  printf(".globl main\n");
  printf("main:\n");

  PrintAssembly(ast_root);

  printf("  pop rax\n");
  printf("  ret\n");
  return 0;
}
