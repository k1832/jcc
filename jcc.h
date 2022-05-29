/* Copyright 2021 Keita Morisaki. All rights reserved. */

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
typedef struct LVar LVar;
struct LVar {
  LVar *next;
  char *name;
  int len;
  int offset;
};

typedef struct Node Node;
typedef struct ArgsForCall ArgsForCall;
struct ArgsForCall {
  Node *node;
  ArgsForCall *next;
};

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
  ND_FUNC_DECLARATION,
  ND_ADDR,
  ND_DEREF,
} NodeKind;

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

  // for ND_FUNC_CALL, link new token to head
  ArgsForCall *args_linked_list_head;

  char *func_name;
  int func_name_len;
  LVar *locals_linked_list_head;    // link new token to head
  LVar *params_linked_list_head;    // link new token to head
  int next_offset_in_block;
  int argc;                             // for ND_FUNC_CALL, ND_FUNC_DECLARATION
  int num_parameters;                   // for ND_FUNC_DECLARATION
  int val;
  int offset;
};
/*** AST definition ***/


/*** GLOBAL VARIALBES ***/
extern Token *token;       // token currently processed
extern char *user_input;   // whole program
// TODO(k1832): Replace this with a linked-list
extern Node *statements[100];
extern int label_num;
/*** GLOBAL VARIALBES ***/


#endif  // JCC_H_
