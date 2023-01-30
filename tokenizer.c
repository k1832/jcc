/* Copyright 2021 Keita Morisaki. All rights reserved. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#include "./jcc.h"

Token *token;       // token currently processed
char *user_input;   // whole program

/*** tokenizer ***/
static Token *ConnectAndGetNewToken(
    TokenKind kind, Token *current, char *str, int len
  ) {
  Token *new_token = calloc(1, sizeof(Token));
  new_token->kind = kind;
  new_token->str = str;
  new_token->len = len;

  current->next = new_token;
  return new_token;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
// Handling global variables that are pointers.
// They are not necessarily initialized in this function.
void Tokenize() {
  char *char_pointer = user_input;
  Token *head;
  head->next = NULL;
  Token *cur = head;

  while (*char_pointer) {
    if (isspace(*char_pointer)) {
      ++char_pointer;
      continue;
    }

    if (StartsWith(char_pointer, "sizeof") &&
      !IsAlnumOrUnderscore(char_pointer[6])) {
      cur = ConnectAndGetNewToken(TK_RESERVED, cur, char_pointer, 6);
      char_pointer += 6;
      continue;
    }

    if (StartsWith(char_pointer, "return") &&
      !IsAlnumOrUnderscore(char_pointer[6])) {
      cur = ConnectAndGetNewToken(TK_RETURN, cur, char_pointer, 6);
      char_pointer += 6;
      continue;
    }

    if (StartsWith(char_pointer, "if") &&
      !IsAlnumOrUnderscore(char_pointer[2])) {
      cur = ConnectAndGetNewToken(TK_IF, cur, char_pointer, 2);
      char_pointer += 2;
      continue;
    }

    if (StartsWith(char_pointer, "else") &&
      !IsAlnumOrUnderscore(char_pointer[4])) {
      cur = ConnectAndGetNewToken(TK_ELSE, cur, char_pointer, 4);
      char_pointer += 4;
      continue;
    }

    if (StartsWith(char_pointer, "while") &&
      !IsAlnumOrUnderscore(char_pointer[5])) {
      cur = ConnectAndGetNewToken(TK_WHILE, cur, char_pointer, 5);
      char_pointer += 5;
      continue;
    }

    if (StartsWith(char_pointer, "for") &&
      !IsAlnumOrUnderscore(char_pointer[3])) {
      cur = ConnectAndGetNewToken(TK_FOR, cur, char_pointer, 3);
      char_pointer += 3;
      continue;
    }

    if (StartsWith(char_pointer, "int") &&
      !IsAlnumOrUnderscore(char_pointer[3])) {
      cur = ConnectAndGetNewToken(TK_INT, cur, char_pointer, 3);
      char_pointer += 3;
      continue;
    }

    if (StartsWith(char_pointer, "==") ||
      StartsWith(char_pointer, "!=") ||
      StartsWith(char_pointer, "<=") ||
      StartsWith(char_pointer, ">=") ||
      StartsWith(char_pointer, "++") ||
      StartsWith(char_pointer, "--") ||
      StartsWith(char_pointer, "+=") ||
      StartsWith(char_pointer, "-=") ||
      StartsWith(char_pointer, "*=") ||
      StartsWith(char_pointer, "/=") ||
      StartsWith(char_pointer, "%=")) {
      cur = ConnectAndGetNewToken(TK_RESERVED, cur, char_pointer, 2);
      char_pointer += 2;
      continue;
    }

    if (('a' <= *char_pointer && *char_pointer <= 'z') ||
      ('A' <= *char_pointer && *char_pointer <= 'Z')) {
      char *start_at = char_pointer;
      while (IsAlnumOrUnderscore(*char_pointer)) {
        ++char_pointer;
      }
      cur = ConnectAndGetNewToken(
        TK_IDENT, cur, start_at, char_pointer-start_at);
      continue;
    }

    if (strchr(";=+-*/()><{},%&", *char_pointer)) {
      cur = ConnectAndGetNewToken(TK_RESERVED, cur, char_pointer++, 1);
      continue;
    }

    if (isdigit(*char_pointer)) {
      // len is temporarily set to 0
      cur = ConnectAndGetNewToken(TK_NUM, cur, char_pointer, 0);

      char *num_start = char_pointer;
      cur->val = strtol(char_pointer, &char_pointer, 10);
      cur->len = char_pointer - num_start;
      continue;
    }

    ExitWithErrorAt(user_input, char_pointer, "Invalid token.");
  }

  ConnectAndGetNewToken(TK_EOF, cur, char_pointer, 1);
  token = head->next;
}
#pragma GCC diagnostic pop
/*** tokenizer ***/
