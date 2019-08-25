#include <stdio.h>

int main(void) {
  int i = 1;
  while (1) {
    i++;
    printf("%d\n", i);
    if (i > 10)
      break;
  }
  return 0;
}
