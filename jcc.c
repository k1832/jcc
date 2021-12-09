/* Copyright 2021 Morisaki, Keita. All rights reserved. */
#include <stdio.h>
#include <stdbool.h>

#include "./jcc.h"


/*** util ***/
bool StartsWith(char *p, char *suffix);

void ExitWithErrorAt(char *loc, char *fmt, ...);
void ExitWithError(char *fmt, ...);
/*** util ***/


/*** tokenizer ***/
Token *ConnectAndGetNewToken(TokenKind kind, Token *cur, char *str, int len);
Token *Tokenize();
/*** tokenizer ***/


/*** token processor ***/
bool ConsumeIfReservedTokenMatches(char *op);
void Expect(char *op);
int ExpectNumber();

bool AtEOF();
/*** token processor ***/


/*** AST parser ***/
Node *NewNode(NodeKind kind);
Node *NewBinary(NodeKind kind, Node *lhs, Node *rhs);
Node *NewNodeNumber(int val);

Node *Expression();
Node *Equality();
Node *Relational();
Node *Add();
Node *MulDiv();
Node *Unary();
Node *Primary();
/*** AST parser ***/


/*** code generator ***/
void PrintAssembly(Node *node);
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
