#include "sicc.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        error("Missing arguments");
    }

    char *arg = argv[1];
    if (!strcmp(arg, "--debug"))
        debug(argv[2]);
    else if (!strcmp(arg, "--dump-ir")) {
        if (argc < 3)
            return 1;
        debug_ir(argv[2]);
        return 0;
    }

    char *s = read_file(arg);
    tokenize(s);
    init_parser();
    node_t *node = parse();
    sema(node);
    ir_t *ir = new_ir();
    gen_ir(ir, node);
    gen_asm(ir);
    return 0;
}
