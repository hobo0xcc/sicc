#include "sicc.h"

#include <stdio.h>
#include <stdlib.h>

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
  buf_t *b = new_buf();
  buf_push(b, 'a');
  buf_append(b, "hello");
  printf("%s\n", buf_str(b));
  exit(0);
}
