#include <stdio.h>

int main(void) {
  int a = 2;
  int i = 0;
  switch (a) {
  case 1:
    i = 20;
    break;
  case 10:
    i = 400;
    break;
  case 2:
    i = 25;
    break;
  default:
    i = 29;
    break;
  }
  return i;
}
