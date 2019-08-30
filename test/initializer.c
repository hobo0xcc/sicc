#include <stdio.h>

enum {
  K_IF,
  K_FOR,
};

struct aaa {
  int ty;
  char *str;
} keywords[] = {
  {K_IF, "if"},
  {K_FOR, "for"},
};

int main(void) {
  for (int i = 0; i < 2; i++) {
    printf("%s\n", keywords[i].str);
  }
  return 0;
}
