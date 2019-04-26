#include "sicc.h"

#include <stdlib.h>
#include <ctype.h>

vec_t *tokens = NULL;

token_t *make_token(int ty, char *str, int line)
{
  token_t *tk = malloc(sizeof(token_t));
  tk->ty = ty;
  tk->str = str;
  tk->line = line;
  return tk;
}

void tokenize(char *s)
{
  char c;
  int n = 0, line = 1;
  tokens = new_vec();

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
    if (c == '+') {
      vec_push(tokens, make_token(TK_PLUS, "+", line));
      continue;
    }
    if (c == '-') {
      vec_push(tokens, make_token(TK_MINUS, "-", line));
      continue;
    }
    if (c == '*') {
      vec_push(tokens, make_token(TK_ASTERISK, "*", line));
      continue;
    }
    if (c == '/') {
      vec_push(tokens, make_token(TK_SLASH, "/", line));
      continue;
    }
    if (c == '(') {
      vec_push(tokens, make_token(TK_LPAREN, "(", line));
      continue;
    }
    if (c == ')') {
      vec_push(tokens, make_token(TK_RPAREN, ")", line));
      continue;
    }

    if (isdigit(c)) {
      str[0] = c;
      for (int i = 1; isdigit(*s); i++)
        str[i] = *s++;

      vec_push(tokens, make_token(TK_NUM, str, line));
      continue;
    }

    error("Unknown character: %c", c);
  }
  vec_push(tokens, make_token(TK_EOF, "\0", line));
}
