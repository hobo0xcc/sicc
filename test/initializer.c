#include <stdio.h>

enum {
  K_IF = 256,
  K_FOR,
};

struct aaa {
  int ty;
  char *str;
} keywords[] = {
  {K_IF, "if"},
  {K_FOR, "for"},
};

struct bbb {
  int a;
  int b;
  int c;
} array[] = {
  {1, 2, 3},
  {4, 5, 6},
  {7, 8, 9},
};

int main(void) {
  for (int i = 0; i < 2; i++) {
    printf("%s, %d\n", keywords[i].str, keywords[i].ty);
  }

  for (int i = 0; i < 3; i++) {
    printf("%d, %d, %d\n", array[i].a, array[i].b, array[i].c);
  }
  return 0;
}
