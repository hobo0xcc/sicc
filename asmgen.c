#include "sicc.h"

#include <stdio.h>
#include <stdlib.h>

void gen_asm()
{
  token_t *t;
  int n = 0;
  while ((t = vec_get(tokens, n))->ty != TK_EOF) {
    n++;
    token_t *peek = vec_get(tokens, n);
    if (t->ty == TK_NUM) {
      printf("  mov rax, %d\n", atoi(t->str));
      continue;
    }

    if (t->ty == TK_PLUS) {
      if (peek->ty != TK_NUM) {
        error("number expected, but got another");
      }
      printf("  add rax, %d\n", atoi(peek->str));
      n++;
      continue;
    }

    if (t->ty == TK_MINUS) {
      if (peek->ty != TK_NUM) {
        error("number expected, but got another");
      }
      printf("  sub rax, %d\n", atoi(peek->str));
      n++;
      continue;
    }

    error("Unknown token type");
  }
}
