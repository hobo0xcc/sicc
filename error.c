#include "sicc.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  fprintf(stderr, "[Error]: ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
  exit(EXIT_FAILURE);
}

void error_at(token_t *tk, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  fprintf(stderr, "[Error]: at (line: %d, pos: %d); ", tk->line, tk->pos);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
  exit(EXIT_FAILURE);
}
