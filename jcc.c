/* Copyright 2021 Morisaki, Keita. All rights reserved. */
#include <stdio.h>
#include <stdbool.h>

#include "./jcc.h"

void *Tokenize();
void BuildAST();
void PrintAssembly(Node *node);

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Exactly one argument must be passed.\n");
    return 1;
  }

  user_input = argv[1];
  Tokenize();
  BuildAST();

  printf(".intel_syntax noprefix\n");
  printf(".globl main\n");

  for (int i = 0; statements[i]; ++i) {
    printf("  # statements[%d] starts.\n", i);
    PrintAssembly(statements[i]);
    printf("  pop rax\n");
  }

  return 0;
}
