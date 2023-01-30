/* Copyright 2021 Keita Morisaki. All rights reserved. */
#include <stdbool.h>

// read this header only once
#ifndef JCC_H_
#define JCC_H_

/*** Token definition ***/
typedef enum {
  TK_RESERVED,
  TK_IDENT,
  TK_NUM,
  TK_EOF,
  TK_RETURN,
  TK_IF,
  TK_ELSE,
  TK_WHILE,
  TK_FOR,
  TK_INT,
} TokenKind;

typedef struct Token Token;
struct Token {
  TokenKind kind;
  Token *next;
  int val;
  char *str;
  int len;
};
/*** Token definition ***/


/*** AST definition ***/
typedef struct Node Node;

typedef enum {
  ND_ADD,
  ND_SUB,
  ND_MUL,
  ND_DIV,
  ND_MOD,
  ND_EQ,
  ND_NEQ,
  ND_LT,
  ND_NGT,
  ND_LVAR,  // local variable
  ND_ASSIGN,
  ND_NUM,
  ND_RETURN,
  ND_IF,
  ND_WHILE,
  ND_FOR,
  ND_BLOCK,
  ND_FUNC_CALL,
  ND_FUNC_DEFINITION,
  ND_ADDR,
  ND_DEREF,
  ND_COMMA,   // Expression, Expression
} NodeKind;

typedef enum {
  TY_PTR,
  TY_INT,
} TypeKind;

typedef struct Type Type;
struct Type {
  TypeKind kind;
  Type *point_to;
};

struct Node {
  NodeKind kind;
  Node *lhs;
  Node *rhs;
  Node *condition;                      // for ND_IF, for ND_FOR
  Node *body_statement;                 // for ND_IF
  Node *else_statement;                 // for ND_IF
  Node *initialization;                 // for ND_FOR
  Node *iteration;                      // for ND_FOR
  Node *next_in_block;                  // for ND_BLOCK

  // function
  char *func_name;
  int func_name_len;
  Node *local_var_next;                 // link new token to head
  Node *param_next;                     // link new token to head
  int next_offset_in_block;
  int argc;                             // for ND_FUNC_CALL, ND_FUNC_DEFINITION
  int num_parameters;                   // for ND_FUNC_DEFINITION
  Type *ret_type;

  // function call
  Node *arg_next;                       // link new token to head
  Node *func_def;

  // variables
  Type *type;  // variable type or return value type
  char *var_name;
  int var_name_len;
  int val;
  int offset;
};
/*** AST definition ***/


/*** GLOBAL VARIALBES ***/
extern Token *token;       // token currently processed
extern char *user_input;   // whole program
// TODO(k1832): Replace this with a linked-list
extern Node *programs[100];
extern int label_num;
extern Type *ty_int;
/*** GLOBAL VARIALBES ***/

void Tokenize();
void BuildAST();
void PrintAssembly(Node *node);
void ExitWithErrorAt(char *input, char *loc, char *fmt, ...);
void ExitWithError(char *fmt, ...);
bool StartsWith(char *p, char *possible_suffix);
bool IsAlnumOrUnderscore(char c);

// type.c
bool IsTypeToken();
Type *GetType();
void AddType(Node *node);
int GetSize(Type *ty);

#endif  // JCC_H_
