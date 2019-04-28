#ifndef SICC_H
#define SICC_H

#include <stdio.h>
#include <stdbool.h>

enum {
  TK_EOF = 256,
  TK_NUM,
  TK_IDENT,
  TK_PLUS,
  TK_MINUS,
  TK_ASTERISK,
  TK_SLASH,
  TK_ASSIGN,
  TK_GREATER,
  TK_LESS,

  TK_LPAREN,
  TK_RPAREN,
  TK_LBRACE,
  TK_RBRACE,
  TK_SEMICOLON,
  TK_COMMA,

  TK_RETURN,
  TK_IF,
  TK_ELSE,
};

enum {
  ND_FUNC = 256,
  ND_FUNCS,
  ND_ARGS,
  ND_PARAMS,
  ND_STMTS,
  ND_NUM,
  ND_IDENT,
  ND_FUNC_CALL,
  ND_EXPR,
  ND_VAR_ASSIGN,
  ND_RETURN,
  ND_IF,
  ND_ELSE,
};

typedef struct _vec {
  int cap, len;
  void **data;
} vec_t;

typedef struct _map {
  int len;
  vec_t *keys;
  vec_t *items;
} map_t;

typedef struct _buf {
  int len, cap;
  char *data;
} buf_t;

typedef struct _token {
  int ty;
  char *str;
  int line;
} token_t;

typedef struct _node {
  int ty;
  struct _node *lhs;
  struct _node *rhs;
  char *str;
  int num;
  vec_t *stmts;
  vec_t *expr;
  vec_t *funcs;
  vec_t *args;
  vec_t *params;
  struct _node *else_stmt;

  bool if_else;
} node_t;

extern vec_t *tokens;
extern map_t *vars;

/* util.c */
vec_t *new_vec();
void grow_vec(vec_t *v, int len);
void vec_push(vec_t *v, void *p);
void vec_append(vec_t *v, int len, ...);
void vec_set(vec_t *v, int pos, void *p);
void *vec_get(vec_t *v, int pos);
size_t vec_len(vec_t *v);

map_t *new_map();
void map_put(map_t *m, char *key, void *item);
void map_set(map_t *m, char *key, void *item);
void *map_get(map_t *m, char *key);
int map_find(map_t *m, char *key);
int map_index(map_t *m, char *key);
size_t map_len(map_t *m);

buf_t *new_buf();
void grow_buf(buf_t *b, int len);
void buf_push(buf_t *b, char c);
void buf_append(buf_t *b, char *str);
size_t buf_len(buf_t *b);
char *buf_str(buf_t *b);

/* debug.c */
void debug();

/* tokenize.c */
void tokenize(char *s);

/* parse.c */
node_t *new_node(int ty);
node_t *parse();

/* asmgen.c */
void gen_asm(node_t *node);

/* error.c */
void error(const char *fmt, ...);

#endif
