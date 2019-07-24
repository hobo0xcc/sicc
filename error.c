#include "sicc.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void error(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(EXIT_FAILURE);
}
