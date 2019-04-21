#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

enum {
  TK_EOF = 256,
  TK_NUM,
};

typedef struct _token {
  int ty;
  char *str;
  int line;
} token_t;

static token_t *tk[100];

token_t *make_token(int ty, char *str, int line)
{
  token_t *tk = malloc(sizeof(token_t));
  tk->ty = ty;
  tk->str = str;
  tk->line = line;
  return tk;
}

void tokenize(char *s);
void error(const char *fmt, ...);

void tokenize(char *s)
{
  char c;
  int n = 0, line = 1;
  while ((c = *s)) {
    s++;
    char *str = calloc(1, sizeof(char) * 5);
    if (c == '\n') {
      line++;
      continue;
    }

    if (c == ' ') {
      continue;
    }

    if (isdigit(c)) {
      str[0] = c;
      for (int i = 1; isdigit(*s); i++)
        str[i] = *s++;

      tk[n++] = make_token(TK_NUM, str, line);
      continue;
    }

    error("Unknown character: %c", c);
  }
  tk[n++] = make_token(TK_EOF, "\0", line);
}

void gen_asm()
{
  token_t *t;
  int n = 0;
  while ((t = tk[n])->ty != TK_EOF) {
    n++;
    if (t->ty == TK_NUM) {
      printf("  mov rax, %d\n", atoi(t->str));
      continue;
    }

    error("Unknown token type");
  }
}

void error(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
  exit(1);
}

int main(int argc, char **argv)
{
  if (argc < 2) {
    error("Missing arguments");
  }

  char *arg = argv[1];
  tokenize(arg);

  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main: \n");
  gen_asm();
  printf("  ret\n");
  
  return 0;
}
