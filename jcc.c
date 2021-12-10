/* Copyright 2021 Morisaki, Keita. All rights reserved. */
#include <stdio.h>
#include <stdbool.h>

#include "./jcc.h"

Token *Tokenize();
Node *Expression();
void PrintAssembly(Node *node);

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
