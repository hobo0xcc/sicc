#include "sicc.h"

#include <stdio.h>
#include <stdlib.h>

static const char *regs[6] = { "rax", "rbx", "rcx", "rdx", "rsi", "rdi" };
static int used = 0;

map_t *vars;
static int vstack = 0;

static int nlabel = 0;

void gen_asm(node_t *node)
{
  if (node->ty == ND_STMTS) {
    for (int i = 0; i < vec_len(node->stmts); i++) {
      gen_asm(vec_get(node->stmts, i));
    }
    return;
  }
  else if (node->ty == ND_RETURN) {
    gen_asm(node->lhs);
    if (used != 1) {
      printf("  mov rax, %s\n", regs[used - 1]);
    }
    printf("  pop rbp\n");
    printf("  ret\n");
    return;
  }
  else if (node->ty == ND_IF) {
    gen_asm(node->rhs);
    printf("  test %s, %s\n", regs[used - 1], regs[used - 1]);
    used--;
    printf("  jz .L%d\n", nlabel);
    gen_asm(node->lhs);
    printf("  .L%d:\n", nlabel);
    nlabel++;
    return;
  }
  else if (node->ty == ND_NUM) {
    if (used > 5)
      error("Insufficient registers");
    printf("  mov %s, %d\n", regs[used++], node->num);
    return;
  }
  else if (node->ty == ND_VAR_ASSIGN) {
    if (map_find(vars, node->str))
      return;
    vstack += 8;
    int *stack = calloc(1, sizeof(int));
    *stack = vstack;
    map_put(vars, node->str, stack);
    return;
  }
  else if (node->ty == ND_IDENT) {
    if (used > 5)
      error("Insufficient regisuters");
    printf("  mov %s, [rbp - %d]\n", regs[used++], *(int *)map_get(vars, node->str));
    return;
  }
  else if (node->ty == ND_EXPR) {
    gen_asm(vec_get(node->expr, 0));
    gen_asm(vec_get(node->expr, 2));
    int op = ((node_t *)vec_get(node->expr, 1))->ty;
    switch (op) {
      case '+':
        printf("  add %s, %s\n", regs[used - 2], regs[used - 1]);
        used--;
        break;
      case '-':
        printf("  sub %s, %s\n", regs[used - 2], regs[used - 1]);
        used--;
        break;
      case '*':
        if (used != 2) {
          printf("  push rax\n");
          printf("  push rdx\n");
          printf("  mov rax, %s\n", regs[used - 2]);
        }
        else {
          printf("  push rdx\n");
        }
        printf("  mul %s\n", regs[used - 1]);
        printf("  add rdx, rax\n");
        printf("  mov %s, rdx\n", regs[used - 2]);
        printf("  pop rdx\n");
        if (used != 2)
          printf("  pop rax\n");
        used--;
        break;
      case '/':
        if (used != 2) {
          printf("  push rax\n");
          printf("  push rdx\n");
          printf("  mov rax, %s\n", regs[used - 2]);
        }
        else {
          printf("  push rdx\n");
        }
        printf("  cdq\n");
        printf("  div %s\n", regs[used - 1]);
        if (used != 2) {
          printf("  mov %s, rax\n", regs[used - 2]);
          printf("  pop rdx\n");
          printf("  pop rax\n");
        }
        else {
          printf("  pop rdx\n");
        }
        used--;
        break;
      case '=':
        {
          char *varname = ((node_t *)vec_get(node->expr, 0))->str;
          printf("  mov qword ptr [rbp - %d], %s\n", *(int *)map_get(vars, varname), regs[used - 1]);
        }
        used--;
        break;
      default:
        error("Unknown operator: %d", op);
    }
    return;
  }
}
