#include "sicc.h"

#include <stdio.h>
#include <stdlib.h>

#define CASE(s) case s: {
#define END                                                                    \
    break;                                                                     \
    }

void debug_tokens(vec_t *tokens) {
    int len = vec_len(tokens);
    for (int i = 0; i < len; i++) {
        token_t *tk = vec_get(tokens, i);
        printf("[%s]: %d\n", tk->str, tk->ty);
    }
}

void debug_node(node_t *node) {
    //   if (node->type != NULL) {
    //     printf("type: %d\n", node->type->ty);
    //     printf("size: %d\n", node->type->size);
    //     printf("ptrsize: %d\n", node->type->ptr_size);
    //   }
    //   switch(node->ty) {
    //     CASE(ND_FUNC)
    //
    //     END
    //   }
}

void debug_ir(char *src) {
    tokenize(src);
    init_parser();
    node_t *node = parse();
    sema(node);
    ir_t *ir = new_ir();
    gen_ir(ir, node);
    print_ir(ir);
}

void debug(char *s) {
    char *str = read_file(s);
    char *p = preprocess(str);
    printf("%s\n", p);
}
