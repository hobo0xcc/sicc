#include <stdio.h>
#include <stdlib.h>

int main(void) {
  int a = 1 + (2 + 3 + 4 + (6 + 7 + 8 + (9 + 10 + 11) + (9 + 2 + (10 + 55 + (8 + 2 + (9 + 2 + (2 + 2 + (2 + 4 + (5 + 5)))))))));
  printf("%d\n", a);
  return 0;
}
