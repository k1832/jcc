/* Copyright 2021 Keita Morisaki. All rights reserved. */
#include <stdio.h>
#include <stdbool.h>

#include "./jcc.h"

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

  for (int i = 0; programs[i]; ++i) {
    printf("  # programs[%d] starts.\n", i);
    if (PrintAssembly(programs[i])) {
      /*
       * "pop" if there is any remaining value at the top
       * to prevent stack overflow
       */
      printf("  pop rax\n");
    }
  }

  if (!globals)
    return 0;

  DBGPRNT;
  printf("\n");
  printf(".data\n");
  for (Node *var = globals->variable_next; var; var=var->variable_next) {
    printf("%.*s:\n", var->var_name_len, var->var_name);

    // TODO(k1832): Replace with GetSize
    int size = 8;
    if (var->type->array_size) {
      size *= var->type->array_size;
    }
    printf("  .zero %d\n", size);
  }

  return 0;
}
