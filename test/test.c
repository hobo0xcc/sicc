#include <stdio.h>
#include <stdlib.h>

int assert_eq(char *fmt, int c) {
  if (!c) {
    printf("PANIC: '%s'", fmt);
    exit(1);
  }
  else
    printf("PASS: '%s'", fmt);
  return 1;
}
