/* Copyright 2021 Morisaki, Keita. All rights reserved. */
#include <stdio.h>

#include "./jcc.h"

void ExitWithError(char *fmt, ...);

// Push the ADDRESS of node only if node is left-value.
void PrintAssemblyForLeftVar(Node *node) {
  if (node->kind != ND_LVAR)
    ExitWithError("Left-hand-side of an assignment is not a variable.");

  printf("  mov rax, rbp\n");
  printf("  sub rax, %d\n", node->offset);
  printf("  push rax\n");
}

// Push processed result (VALUE) of node.
void PrintAssembly(Node *node) {
  switch (node->kind) {
    case ND_NUM:
      printf("  push %d\n", node->val);
      return;
    case ND_LVAR:
      PrintAssemblyForLeftVar(node);
      printf("  pop rax\n");
      printf("  mov rax, [rax]\n");
      printf("  push rax\n");
      return;
    case ND_ASSIGN:
      // Assign right to left and push right value to the stack.
      PrintAssemblyForLeftVar(node->lhs);
      PrintAssembly(node->rhs);

      printf("  pop rdi\n");
      printf("  pop rax\n");
      printf("  mov [rax], rdi\n");
      printf("  push rdi\n");
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