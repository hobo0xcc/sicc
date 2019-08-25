#include <stdio.h>

int main(void) {
  int a = 2 + 8;
  int i = 0;
  switch (a) {
  case 50:
    i = 20;
    break;
  case 10:
    i = 400;
  case 2:
    i = 25;
  default:
    i = 29;
    break;
  }
  return i;
}
