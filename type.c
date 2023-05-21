/* Copyright 2022 Keita Morisaki. All rights reserved. */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "./jcc.h"

Type *ty_int = &(Type){TY_INT};   // NOLINT(readability/braces)

bool IsTypeToken() {
  switch (token->kind) {
  case TK_INT:
    return true;

  default:
    return false;
  }
}

/*
 * Get "Type *" pointing to "point_to"
 */
Type *PointTo(Type *point_to) {
  Type *ty = calloc(1, sizeof(Type));
  ty->kind = TY_PTR;
  ty->point_to = point_to;
  return ty;
}

void AddType(Node *node) {
  if (!node || node->type) {
    return;
  }

  AddType(node->lhs);
  AddType(node->rhs);
  AddType(node->condition);
  AddType(node->body_program);
  AddType(node->else_program);
  AddType(node->initialization);
  AddType(node->iteration);

  for (Node *nd = node->next_in_block; nd; nd=nd->next_in_block) {
    AddType(nd);
  }

  Type *ty_int = calloc(1, sizeof(Type));
  ty_int->kind = TY_INT;
  switch (node->kind) {
    case ND_ADD:
    case ND_SUB:
    case ND_MUL:
    case ND_DIV:
    case ND_MOD:
    case ND_ASSIGN:
      node->type = node->lhs->type;
      return;
    case ND_EQ:
    case ND_NEQ:
    case ND_LT:
    case ND_NGT:
    case ND_LVAR:
    case ND_NUM:
      node->type = ty_int;
      return;
    case ND_FUNC_CALL:
      node->type = node->func_def->ret_type;
      return;
    case ND_ADDR:
      node->type = PointTo(node->lhs->type);
      return;
    case ND_DEREF:
      if (node->lhs->type->kind == TY_PTR)
        node->type = node->lhs->type->point_to;
      else
        // In case like: "*(&a + 2)"
        node->type = ty_int;
      return;
    case ND_COMMA:
     node->type = node->rhs->type;
     return;
    default:
      return;
  }
}

int GetSize(Type *ty) {
  if (!ty) {
    ExitWithError("Type is not determined for this node.");
    return 0;   // To make cpplint happy
  }

  switch (ty->kind) {
    case TY_PTR:
      return 8;
    case TY_INT:
      return 4;
    default:
      ExitWithError("\"sizeof\" this type is not defined.");
      return 0;   // To make cpplint happy
  }
}
