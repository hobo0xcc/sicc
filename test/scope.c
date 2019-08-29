#include <stdio.h>

int main()
{
  int n = 0;
  while (n < 10) {
    n = n + 1;
    int i = n;
    printf("%d\n", i);
  }
  return n;
}
