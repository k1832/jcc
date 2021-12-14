/* Copyright 2021 Morisaki, Keita. All rights reserved. */
#include <stdio.h>
#include <stdbool.h>

#include "./jcc.h"

void *Tokenize();
void Program();
void PrintAssembly(Node *node);

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Exactly one argument must be passed.\n");
    return 1;
  }

  user_input = argv[1];
  Tokenize();
  Program();

  printf(".intel_syntax noprefix\n");
  printf(".globl main\n");
  printf("main:\n");

  // prologue
  printf("  # %s (%s): at line %d\n", __FILE__, __func__, __LINE__);
  printf("  push rbp\n");
  printf("  mov rbp, rsp\n");
  int num_variables = 26;
  int bytes_per_variable = 8;
  printf("  sub rsp, %d\n", num_variables * bytes_per_variable);

  for (int i = 0; statements[i]; ++i) {
    printf("  # statements[%d] starts.\n", i);
    PrintAssembly(statements[i]);
    printf("  pop rax\n");
  }

  // epilogue
  printf("  # %s (%s): at line %d\n", __FILE__, __func__, __LINE__);
  printf("  mov rsp, rbp\n");
  printf("  pop rbp\n");
  // "ret" pops the address stored at the stack top, and jump there.
  printf("  ret\n");
  return 0;
}
