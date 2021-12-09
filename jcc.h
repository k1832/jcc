/* Copyright 2021 Morisaki, Keita. All rights reserved. */

/*** Token definition ***/
typedef enum {
  TK_RESERVED,
  TK_NUM,
  TK_EOF,
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
  ND_NUM,
} NodeKind;

typedef struct Node Node;
struct Node {
  NodeKind kind;
  Node *lhs;
  Node *rhs;
  int val;
};
/*** AST definition ***/


/*** GLOBAL VARIALBES ***/
extern Token *token;       // token currently processed
extern char *user_input;   // whole program
/*** GLOBAL VARIALBES ***/
