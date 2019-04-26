#include "sicc.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

int main(int argc, char **argv)
{
  if (argc < 2) {
    error("Missing arguments");
  }

  char *arg = argv[1];
  if (!strcmp(arg, "--debug"))
    debug();
  tokenize(arg);
  node_t *node = parse();
  vars = new_map();

  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main: \n");
  printf("  push rbp\n");
  printf("  mov rbp, rsp\n");
  gen_asm(node);
  
  return 0;
}
