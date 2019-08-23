#include <stdio.h>
#include <stdlib.h>

typedef struct {
  char *str;
  int cur;
  int ty;
} Token;

int main(void) {
  Token *tk = malloc(sizeof(Token));
  tk->str = "if";
  tk->cur = 10;
  tk->ty = 267;
  printf("str: %s, cur: %d, ty: %d\n", tk->str, tk->cur, tk->ty);
  return 0;
}
