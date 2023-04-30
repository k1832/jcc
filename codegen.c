/* Copyright 2021 Keita Morisaki. All rights reserved. */
#include <stdio.h>

#include "./jcc.h"

int label_num = 0;
const int label_digit = 5;
static const char registers[6][4] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

// Push the ADDRESS of node only if node is left-value.
static void PrintAssemblyForLeftVar(Node *node) {
  DBGPRNT;

  if (node->kind == ND_DEREF) {
    /*
     * E.g.
     * int a; int *tmp;
     * tmp = &a;
     * tmp = 10; // This assignment
     *
     * The address of "a" is stored in "tmp" as a value
     * in the example above
     */
    PrintAssembly(node->lhs);
    return;
  }

  if (node->kind != ND_LVAR) {
    ExitWithError("Left-hand-side of an assignment is not a variable.");
  }

  printf("  mov rax, rbp\n");
  printf("  sub rax, %d\n", node->offset);
  printf("  push rax\n");
}

// Push processed result (VALUE) of node.
void PrintAssembly(Node *node) {
  if (node == NULL) {
    ExitWithError("Can not print assembly for NULL.\n");
  }

  // TODO(k1832): Use Switch-case
  if (node->kind == ND_NUM) {
    DBGPRNT;
    printf("  push %d\n", node->val);
    return;
  }

  if (node->kind == ND_LVAR) {
    DBGPRNT;
    PrintAssemblyForLeftVar(node);
    DBGPRNT;
    printf("  pop rax\n");
    printf("  mov rdi, [rax]\n");
    printf("  push rdi\n");

    // if (!node->post_increment && !node->post_decrement) {
    //   return;
    // }

    // // Increment or decrement after original value is pushed to stack
    // if (node->post_increment && node->post_decrement) {
    //   ExitWithError(
    //     "Both post increment & decrement cannot happen at the same time.");
    // }
    // if (node->post_increment) {
    //   printf("  add rdi, 1\n");
    // } else if (node->post_decrement) {
    //   printf("  sub rdi, 1\n");
    // }
    // printf("  mov [rax], rdi\n");
    return;
  }

  if (node->kind == ND_ASSIGN) {
    // Assign right to left and push right value to the stack.
    DBGPRNT;
    PrintAssemblyForLeftVar(node->lhs);
    PrintAssembly(node->rhs);
    printf("  pop rdi\n");
    printf("  pop rax\n");
    printf("  mov [rax], rdi\n");
    printf("  push rdi\n");
    return;
  }

  if (node->kind == ND_RETURN) {
    DBGPRNT;
    PrintAssembly(node->lhs);
    printf("  pop rax\n");
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    // "ret" pops the address stored at the stack top, and jump there.
    printf("  ret\n");
    return;
  }
  if (node->kind == ND_IF) {
    DBGPRNT;
    int label_for_else_statement = label_num++;
    int label_for_if_end = label_num++;
    PrintAssembly(node->condition);
    printf("  pop rax\n");  // pop condition
    printf("  cmp rax, 0\n");
    // if condition is false, skip the if (body) statement
    printf("  je .L%0*d\n", label_digit, label_for_else_statement);

    PrintAssembly(node->body_statement);
    printf("  jmp .L%0*d\n", label_digit, label_for_if_end);

    printf(".L%0*d:\n", label_digit, label_for_else_statement);
    if (node->else_statement) {
      PrintAssembly(node->else_statement);
    }

    printf(".L%0*d:\n", label_digit, label_for_if_end);
    return;
  }

  if (node->kind == ND_WHILE) {
    DBGPRNT;
    int label_for_while_start = label_num++;
    int label_for_while_end = label_num++;
    printf(".L%0*d:\n", label_digit, label_for_while_start);
    PrintAssembly(node->lhs);
    printf("  pop rax\n");  // pop condition
    printf("  cmp rax, 0\n");
    // if condition is false, skip the while statement
    printf("  je .L%0*d\n", label_digit, label_for_while_end);

    PrintAssembly(node->rhs);
    printf("  jmp .L%0*d\n", label_digit, label_for_while_start);

    printf(".L%0*d:\n", label_digit, label_for_while_end);
    return;
  }

  if (node->kind == ND_FOR) {
    DBGPRNT;
    int label_for_for_start = label_num++;
    int label_for_for_end = label_num++;
    if (node->initialization) {
      PrintAssembly(node->initialization);
    }
    printf(".L%0*d:\n", label_digit, label_for_for_start);
    if (node->condition == NULL) {
      printf("  push 1\n");  // HACK: condition is always true.
    } else {
      PrintAssembly(node->condition);
    }
    printf("  pop rax\n");  // pop condition
    printf("  cmp rax, 0\n");
    // if condition is false, skip the for statement
    printf("  je .L%0*d\n", label_digit, label_for_for_end);

    PrintAssembly(node->body_statement);
    if (node->iteration) {
      PrintAssembly(node->iteration);
    }
    printf("  jmp .L%0*d\n", label_digit, label_for_for_start);

    printf(".L%0*d:\n", label_digit, label_for_for_end);
    return;
  }

  if (node->kind == ND_BLOCK) {
    DBGPRNT;
    node = node->next_in_block;
    while (node) {
      PrintAssembly(node);
      printf("  pop rax\n");  // pop statement result
      node = node->next_in_block;
    }
    return;
  }

  if (node->kind == ND_FUNC_CALL) {
    DBGPRNT;

    // Push arguments after the first 6 arguments
    // reversely to stack
    int argv_i = node->argc;
    Node *argument = node->arg_next;  // head of arg linked-list
    while (argv_i > 6) {
      PrintAssembly(argument);
      // leave the result in stack

      argument = argument->arg_next;
      --argv_i;
    }
    while (argv_i) {
      // Transfer results to registers specified by ABI.
      PrintAssembly(argument);
      printf("  pop %s\n", registers[argv_i - 1]);

      argument = argument->arg_next;
      --argv_i;
    }
    // TODO(k1832): 16byte allignment?
    printf("  call %.*s\n", node->func_name_len, node->func_name);
    printf("  push rax\n");
    return;
  }

  if (node->kind == ND_FUNC_DEFINITION) {
    DBGPRNT;
    printf("%.*s:\n", node->func_name_len, node->func_name);
    // prologue
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    const int num_variables = 26;
    const int bytes_per_variable = 8;
    printf("  sub rsp, %d\n", num_variables * bytes_per_variable);


    // Transfer argument values into stack frame
    Node *param = node->param_next;  // The first param of the function
    int param_i = node->num_parameters;
    while (param_i > 6) {
      // No need to transfer values for arguments after the first 6 arguments
      // because they are allowed to be out of the stack frame and
      // will be accessed with negative offset.
      param = param->param_next;
      --param_i;
    }

    while (param_i) {
      printf("  mov rax, rbp\n");
      printf("  sub rax, %d\n", param->offset);
      printf("  mov [rax], %s\n", registers[param_i - 1]);
      param = param->param_next;
      --param_i;
    }

    node = node->next_in_block;
    while (node) {
      PrintAssembly(node);
      printf("  pop rax\n");
      node = node->next_in_block;
    }
    // epilogue
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    // "ret" pops the address stored at the stack top, and jump there.
    printf("  ret\n");
    return;
  }

  if (node->kind == ND_ADDR) {
    DBGPRNT;
    PrintAssemblyForLeftVar(node->lhs);
    return;
  }

  if (node->kind == ND_DEREF) {
    DBGPRNT;
    PrintAssembly(node->lhs);
    printf("  pop rax\n");
    printf("  mov rax, [rax]\n");
    printf("  push rax\n");
    return;
  }

  // "Expression A, Expression B"
  // Both expressions are evaluated.
  // And the value of this is Expression B
  if (node->kind == ND_COMMA) {
    PrintAssembly(node->lhs);
    printf("  pop rax\n");
    PrintAssembly(node->rhs);
    return;
  }

  PrintAssembly(node->lhs);
  PrintAssembly(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->kind) {
    case ND_ADD:
      DBGPRNT;
      printf("  add rax, rdi\n");
      break;
    case ND_SUB:
      DBGPRNT;
      printf("  sub rax, rdi\n");
      break;
    case ND_MUL:
      DBGPRNT;
      printf("  imul rax, rdi\n");
      break;
    case ND_DIV:
      DBGPRNT;
      printf("  cqo\n");
      printf("  idiv rdi\n");
      break;
    case ND_MOD:
      DBGPRNT;
      printf("  cqo\n");
      printf("  idiv rdi\n");
      printf("  mov rax, rdx\n");
      break;
    case ND_EQ:
      DBGPRNT;
      printf("  cmp rax, rdi\n");
      printf("  sete al\n");  // al is the bottom 8 bits of rax
      printf("  movzb rax, al\n");  // zero-fill top 56 bits
      break;
    case ND_LT:
      // rax < rdi
      DBGPRNT;
      printf("  cmp rax, rdi\n");
      printf("  setl al\n");
      printf("  movzb rax, al\n");
      break;
    case ND_NGT:
      // rax <= rdi
      DBGPRNT;
      printf("  cmp rax, rdi\n");
      printf("  setle al\n");
      printf("  movzb rax, al\n");
      break;
    default:
      ExitWithError("node->kind %u is not handled in %s",
                    node->kind, __FUNCTION__);
  }

  printf("  push rax\n");
}
