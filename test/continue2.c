#include <stdio.h>

int main(void) {
  int i = 0;
  for (;; i++) {
    if (i < 6)
      continue;
    else
      break;
  }

  return i;
}
