/* Copyright 2021 Keita Morisaki. All rights reserved. */
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*** error ***/
void ExitWithErrorAt(char *input, char *loc, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  int padding_len = loc - input;
  char marker[] = "^ ";
  int marker_len = (int) strlen(marker);
  fprintf(stderr, "%s\n", input);
  fprintf(stderr, "%*s", padding_len + marker_len, marker);
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

bool StartsWith(char *p, char *possible_suffix) {
  return !memcmp(p, possible_suffix, strlen(possible_suffix));
}

bool IsAlnumOrUnderscore(char c) {
  return isalnum(c) || c == '_';
}
