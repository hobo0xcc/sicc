#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

enum {
  TK_EOF = 256,
  TK_NUM,
  TK_PLUS,
  TK_MINUS,
};

typedef struct _token {
  int ty;
  char *str;
  int line;
} token_t;

typedef struct _vec {
  int cap, len;
  void **data;
} vec_t;

typedef struct _map {
  int len;
  vec_t *keys;
  vec_t *items;
} map_t;

vec_t *new_vec();
void grow_vec(vec_t *v, int len);
void vec_push(vec_t *v, void *p);
void *vec_get(vec_t *v, int pos);
int vec_len(vec_t *v);

map_t *new_map();
void map_put(map_t *m, char *key, void *item);
void *map_get(map_t *m, char *key);
int map_index(map_t *m, char *key);
int map_len(map_t *m);

void debug();

vec_t *new_vec()
{
  vec_t *v = malloc(sizeof(vec_t));
  v->len = 0;
  v->cap = sizeof(void *);
  v->data = malloc(sizeof(void *));
  return v;
}

void grow_vec(vec_t *v, int len)
{
  int size = sizeof(void *) * (v->len + len);
  if (size <= v->cap)
    return;
  while (size > v->cap)
    v->cap *= 2;
  v->data = realloc(v->data, v->cap);
}

void vec_push(vec_t *v, void *p)
{
  grow_vec(v, 1);
  v->data[v->len++] = p;
}

void *vec_get(vec_t *v, int pos)
{
  if (pos > v->len || pos < 0)
    return NULL;
  return v->data[pos];
}

int vec_len(vec_t *v)
{
  return v->len;
}

map_t *new_map()
{
  map_t *m = malloc(sizeof(map_t));
  m->len = 0;
  m->keys = new_vec();
  m->items = new_vec();
  return m;
}

void map_put(map_t *m, char *key, void *item)
{
  vec_push(m->keys, key);
  vec_push(m->items, item);
  m->len++;
}

void *map_get(map_t *m, char *key)
{
  int i;
  if ((i = map_index(m, key)) == -1)
    return NULL;
  return vec_get(m->items, i);
}

int map_index(map_t *m, char *key)
{
  for (int i = 0; i < m->len; i++) {
    if (!strcmp(vec_get(m->keys, i), key))
      return i;
  }
  return -1;
}

int map_len(map_t *m)
{
  return m->len;
}

void debug()
{
  vec_t *v = new_vec();
  vec_push(v, "a");
  vec_push(v, "b");
  printf("%s\n", vec_get(v, 1));
  map_t *m = new_map();
  map_put(m, "a", "eax");
  map_put(m, "b", "edi");
  printf("%s\n", map_get(m, "a"));
  exit(0);
}

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

    if (c == '+') {
      tk[n++] = make_token(TK_PLUS, "+", line);
      continue;
    }

    if (c == '-') {
      tk[n++] = make_token(TK_MINUS, "-", line);
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

    if (t->ty == TK_PLUS) {
      if (tk[n]->ty != TK_NUM) {
        error("number expected, but got another");
      }
      printf("  add rax, %d\n", atoi(tk[n++]->str));
      continue;
    }

    if (t->ty == TK_MINUS) {
      if (tk[n]->ty != TK_NUM) {
        error("number expected, but got another");
      }
      printf("  sub rax, %d\n", atoi(tk[n++]->str));
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
  if (!strcmp(arg, "--debug"))
    debug();
  tokenize(arg);

  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main: \n");
  gen_asm();
  printf("  ret\n");
  
  return 0;
}
