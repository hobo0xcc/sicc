#include "sicc.h"

#include <stdio.h>
#include <stdlib.h>

#define CASE(s) case s: {
#define END break; }

void debug_tokens(vec_t *tokens)
{
  int len = vec_len(tokens);
  for (int i = 0; i < len; i++) {
    token_t *tk = vec_get(tokens, i);
    printf("[%s]: %d\n", tk->str, tk->ty);
  }
}

void debug_node(node_t *node)
{
  if (node->type != NULL) {
    printf("type: %d\n", node->type->ty);
    printf("size: %d\n", node->type->size);
    printf("ptrsize: %d\n", node->type->ptr_size);
  }
  switch(node->ty) {
    CASE(ND_FUNC)
      
    END
  }
}

void debug_ir(char *src)
{
  tokenize(src);
  init_parser();
  node_t *node = parse();
  sema(node);
  ir_t *ir = new_ir();
  gen_ir(ir, node);
  print_ir(ir);
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
  buf_t *b = new_buf();
  buf_push(b, 'a');
  buf_append(b, "hello");
  printf("%s\n", buf_str(b));
  exit(0);
}
