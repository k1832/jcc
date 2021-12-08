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
};
/*** Token definition ***/


/*** AST definition ***/
typedef enum {
  ND_ADD,
  ND_SUB,
  ND_MUL,
  ND_DIV,
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
Token *ConnectAndGetNewToken(TokenKind kind, Token *cur, char *str) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
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

    if (strchr("+-*/()", *char_pointer)) {
      cur = ConnectAndGetNewToken(TK_RESERVED, cur, char_pointer++);
      continue;
    }

    if (isdigit(*char_pointer)) {
      cur = ConnectAndGetNewToken(TK_NUM, cur, char_pointer);
      cur->val = strtol(char_pointer, &char_pointer, 10);
      continue;
    }

    ExitWithErrorAt(char_pointer, "Invalid token.");
  }

  ConnectAndGetNewToken(TK_EOF, cur, char_pointer);
  return head.next;
}
/*** tokenizer ***/


/*** token processor ***/
bool ConsumeIfReservedTokenMatches(char op) {
  if (token->kind != TK_RESERVED || token->str[0] != op)
    return false;

  token = token->next;
  return true;
}

void Expect(char op) {
  if (token->kind != TK_RESERVED || token->str[0] != op)
    ExitWithErrorAt(token->str, "Expected %c.", op);
  token = token->next;
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
Node *MulDiv();
Node *Unary();
Node *Primary();

// Expression    = MulDiv ("+" MulDiv | "-" MulDiv)*
Node *Expression() {
  Node *node = MulDiv();
  for (;;) {
    if (ConsumeIfReservedTokenMatches('+')) {
      node = NewBinary(ND_ADD, node, MulDiv());
      continue;
    }

    if (ConsumeIfReservedTokenMatches('-')) {
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
    if (ConsumeIfReservedTokenMatches('*')) {
      node = NewBinary(ND_MUL, node, Unary());
      continue;
    }

    if (ConsumeIfReservedTokenMatches('/')) {
      node = NewBinary(ND_DIV, node, Unary());
      continue;
    }

    return node;
  }
}

// Unary   = ("+" | "-")? Primary
Node *Unary() {
  if (ConsumeIfReservedTokenMatches('+'))
    return Primary();

  if (ConsumeIfReservedTokenMatches('-'))
    return NewBinary(ND_SUB, NewNodeNumber(0), Primary());

  return Primary();
}

// Primary = number | "(" Expression ")"
Node *Primary() {
  if (ConsumeIfReservedTokenMatches('(')) {
    Node *node = Expression();
    Expect(')');
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
  }

  printf("  push rax\n");
}
/*** code generator ***/


int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "引数の個数が正しくありません\n");
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
