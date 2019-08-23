#include <stdio.h>

int main(void) {
  int a = 0;
hello:
  a++;
  printf("Hello\n");
  if (a < 10) goto hello;
  return 0;
}
