#ifndef SICC_H
#define SICC_H

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

extern vec_t *tokens;

/* util.c */
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

/* debug.c */
void debug();

/* tokenize.c */
void tokenize(char *s);

/* asmgen.c */
void gen_asm();

/* error.c */
void error(const char *fmt, ...);

#endif
