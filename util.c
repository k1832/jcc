/* Copyright 2021 Morisaki, Keita. All rights reserved. */
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "./jcc.h"

char *user_input;   // whole program

/*** error ***/
void ExitWithErrorAt(char *loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  int pos = loc - user_input;
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s", pos, " ");
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

void ExitWithError(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}
/*** error ***/

bool StartsWith(char *p, char *suffix) {
  return !memcmp(p, suffix, strlen(suffix));
}
