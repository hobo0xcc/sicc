#include "sicc.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

void error(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
  exit(1);
}
