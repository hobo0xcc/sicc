#include "sicc.h"

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

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

void vec_append(vec_t *v, int len, ...)
{
  va_list ap;
  va_start(ap, len);

  for (int i = 0; i < len; i++) {
    vec_push(v, va_arg(ap, void *));
  }
  va_end(ap);
}

void vec_set(vec_t *v, int pos, void *p)
{
  if (pos > v->len || pos < 0)
    return;
  v->data[pos] = p;
}

void *vec_get(vec_t *v, int pos)
{
  if (pos > v->len || pos < 0)
    return NULL;
  return v->data[pos];
}

size_t vec_len(vec_t *v)
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

void map_set(map_t *m, char *key, void *item)
{
  int i = map_index(m, key);
  if (i == -1)
    return;
  vec_set(m->items, i, item);
}

void *map_get(map_t *m, char *key)
{
  int i;
  if ((i = map_index(m, key)) == -1)
    return NULL;
  return vec_get(m->items, i);
}

int map_find(map_t *m, char *key)
{
  if (map_index(m, key) == -1)
    return 0;
  return 1;
}

int map_index(map_t *m, char *key)
{
  for (int i = 0; i < m->len; i++) {
    if (!strcmp(vec_get(m->keys, i), key))
      return i;
  }
  return -1;
}

size_t map_len(map_t *m)
{
  return m->len;
}

buf_t *new_buf()
{
  buf_t *b = calloc(1, sizeof(buf_t));
  b->len = 0;
  b->cap = sizeof(char);
  b->data = malloc(sizeof(char));
  return b;
}

void grow_buf(buf_t *b, int len)
{
  int blen = b->len + len;
  if (b->cap >= blen)
    return;
  while (blen > b->cap)
    b->cap *= 2;
  b->data = realloc(b->data, b->cap);
}

void buf_push(buf_t *b, char c)
{
  grow_buf(b, 1);
  b->data[b->len++] = c;
}

void buf_append(buf_t *b, char *str)
{
  int len = strlen(str);
  for (int i = 0; i < len; i++)
    buf_push(b, str[i]);
}

size_t buf_len(buf_t *b)
{
  return b->len;
}

char *buf_str(buf_t *b)
{
  b->data[b->len] = '\0';
  return b->data;
}