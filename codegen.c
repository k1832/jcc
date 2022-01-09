/* Copyright 2021 Morisaki, Keita. All rights reserved. */
#include <stdio.h>

#include "./jcc.h"

void ExitWithError(char *fmt, ...);

int label_num = 0;

// Push the ADDRESS of node only if node is left-value.
void PrintAssemblyForLeftVar(Node *node) {
  printf("  # %s (%s): at line %d\n", __FILE__, __func__, __LINE__);

  if (node->kind != ND_LVAR) {
    ExitWithError("Left-hand-side of an assignment is not a variable.");
  }

  printf("  mov rax, rbp\n");
  printf("  sub rax, %d\n", node->offset);
  printf("  push rax\n");
}

// Push processed result (VALUE) of node.
void PrintAssembly(Node *node) {
  switch (node->kind) {
    case ND_NUM:
      printf("  # %s (%s): at line %d\n", __FILE__, __func__, __LINE__);
      printf("  push %d\n", node->val);
      return;
    case ND_LVAR:
      printf("  # %s (%s): at line %d\n", __FILE__, __func__, __LINE__);
      PrintAssemblyForLeftVar(node);
      printf("  pop rax\n");
      printf("  mov rax, [rax]\n");
      printf("  push rax\n");
      return;
    case ND_ASSIGN:
      // Assign right to left and push right value to the stack.
      printf("  # %s (%s): at line %d\n", __FILE__, __func__, __LINE__);
      PrintAssemblyForLeftVar(node->lhs);
      PrintAssembly(node->rhs);
      printf("  pop rdi\n");
      printf("  pop rax\n");
      printf("  mov [rax], rdi\n");
      printf("  push rdi\n");
      return;
    case ND_RETURN:
      printf("  # %s (%s): at line %d\n", __FILE__, __func__, __LINE__);
      PrintAssembly(node->lhs);
      printf("  pop rax\n");
      printf("  mov rsp, rbp\n");
      printf("  pop rbp\n");
      // "ret" pops the address stored at the stack top, and jump there.
      printf("  ret\n");
      return;
    case ND_IF:
      printf("  # %s (%s): at line %d\n", __FILE__, __func__, __LINE__);
      PrintAssembly(node->lhs);
      printf("  pop rax\n");  // pop condition
      printf("  cmp rax, 0\n");   // if condition is false
      // max digits: 3
      printf("  je .L%03d\n", label_num);
      PrintAssembly(node->rhs);   // if condition is false, this will run
      printf(".L%03d:\n", label_num);
      ++label_num;
      return;
  }

  PrintAssembly(node->lhs);
  PrintAssembly(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->kind) {
    case ND_ADD:
      printf("  # %s (%s): at line %d\n", __FILE__, __func__, __LINE__);
      printf("  add rax, rdi\n");
      break;
    case ND_SUB:
      printf("  # %s (%s): at line %d\n", __FILE__, __func__, __LINE__);
      printf("  sub rax, rdi\n");
      break;
    case ND_MUL:
      printf("  # %s (%s): at line %d\n", __FILE__, __func__, __LINE__);
      printf("  imul rax, rdi\n");
      break;
    case ND_DIV:
      printf("  # %s (%s): at line %d\n", __FILE__, __func__, __LINE__);
      printf("  cqo\n");
      printf("  idiv rdi\n");
      break;
    case ND_EQ:
      printf("  # %s (%s): at line %d\n", __FILE__, __func__, __LINE__);
      printf("  cmp rax, rdi\n");
      printf("  sete al\n");  // al is the bottom 8 bits of rax
      printf("  movzb rax, al\n");  // zero-fill top 56 bits
      break;
    case ND_LT:
      // rax < rdi
      printf("  # %s (%s): at line %d\n", __FILE__, __func__, __LINE__);
      printf("  cmp rax, rdi\n");
      printf("  setl al\n");
      printf("  movzb rax, al\n");
      break;
    case ND_NGT:
      // rax <= rdi
      printf("  # %s (%s): at line %d\n", __FILE__, __func__, __LINE__);
      printf("  cmp rax, rdi\n");
      printf("  setle al\n");
      printf("  movzb rax, al\n");
      break;
  }

  printf("  push rax\n");
}
