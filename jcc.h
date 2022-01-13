/* Copyright 2021 Morisaki, Keita. All rights reserved. */

// read this header only once
#ifndef JCC_H_
#define JCC_H_


/*** Token definition ***/
typedef enum {
  // TODO(k1832): change TK_RESERVED to TK_OPERATER,
  TK_RESERVED,
  TK_IDENT,
  TK_NUM,
  TK_EOF,
  TK_RETURN,
  TK_IF,
  TK_ELSE,
  TK_WHILE,
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
typedef enum {
  ND_ADD,
  ND_SUB,
  ND_MUL,
  ND_DIV,
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
} NodeKind;

typedef struct Node Node;
struct Node {
  NodeKind kind;
  Node *lhs;
  Node *rhs;
  Node *condition;        // for ND_IF, for ND_FOR
  Node *body_statement;     // for ND_IF
  Node *else_statement;   // for ND_IF
  int val;
  int offset;
};
/*** AST definition ***/


typedef struct LVar LVar;
struct LVar {
  LVar *next;
  char *name;
  int len;
  int offset;
};


/*** GLOBAL VARIALBES ***/
extern Token *token;       // token currently processed
extern char *user_input;   // whole program
extern Node *statements[100];
extern LVar *locals_linked_list_head;   // link new token to head
extern int label_num;
/*** GLOBAL VARIALBES ***/


#endif  // JCC_H_
