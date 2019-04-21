#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

void error(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
  exit(1);
}

int main(int argc, char **argv)
{
  if (argc < 2) {
    error("Missing arguments");
  }

  char *arg = argv[1];

  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main: \n");
  printf("  mov rax, %d\n", arg[0] - '0');
  printf("  ret\n");
  
  return 0;
}
