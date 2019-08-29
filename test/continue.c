#include <stdio.h>

int main(void) {
  int i = 0;
  while (1) {
    i++;
    if (i < 5)
      continue;
    else
      break;
  }

  return i;
}
