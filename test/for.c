#include <stdio.h>

int main(void) {
  for (int i = 0; i < 10; i++) {
    printf("%d\n", i);
  }
  int i = 41;
  printf("%d\n", i++);
  return i;
}
