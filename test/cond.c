#include <stdio.h>

int main(void) {
  int a = 1, b = 1;
  if (a && b)
    printf("Oh!\n");
  int c = 0;
  if (a || c)
    printf("Yeah!\n");
  int d = a ? 10 : 20;
  return d;
}
