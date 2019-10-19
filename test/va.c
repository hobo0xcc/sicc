#include <stdarg.h>
#include <stdio.h>

int print(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stdout, fmt, ap);
  fprintf(stdout, "\n");
  va_end(ap);
  return 0;
}

int main(void) {
  char *s;
  printf("Enter your name!\n");
  scanf("%s", s);
  print("Hello %s!", s);
  return 0;
}
