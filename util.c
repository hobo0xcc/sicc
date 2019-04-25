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
