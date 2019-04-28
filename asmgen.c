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
  if (node->ty == ND_FUNCS) {
    for (int i = 0; i < vec_len(node->funcs); i++) {
      printf(".global _%s\n", ((node_t *)vec_get(node->funcs, i))->str);
    }
    for (int i = 0; i < vec_len(node->funcs); i++) {
      gen_asm(vec_get(node->funcs, i));
    }
    return;
  }
  else if (node->ty == ND_ARGS) {
    int pushed_arg = vec_len(node->args) - 5;
    int offset = 0;
    if (pushed_arg > 0)
      offset = pushed_arg * 8 + 8; // '+ 8' = rbp size

    for (int i = 0; i < vec_len(node->args); i++) {
      node_t *var = vec_get(node->args, i);
      gen_asm(var);
      switch (i) {
        case 0:
          printf("  mov [rbp - %d], rdi\n", vstack);
          break;
        case 1:
          printf("  mov [rbp - %d], rsi\n", vstack);
          break;
        case 2:
          printf("  mov [rbp - %d], rdx\n", vstack);
          break;
        case 3:
          printf("  mov [rbp - %d], rcx\n", vstack);
          break;
        case 4:
          printf("  mov [rbp - %d], r8\n", vstack);
          break;
        case 5:
          printf("  mov [rbp - %d], r9\n", vstack);
          break;
        default:
          printf("  mov %s, [rbp + %d]\n", regs[used], offset -= 8);
          printf("  mov [rbp - %d], %s\n", vstack, regs[used]);
          break;
      }
    }
    return;
  }
  else if (node->ty == ND_FUNC) {
    printf("_%s:\n", node->str);
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    gen_asm(node->rhs);
    gen_asm(node->lhs);
    vec_t *stmts = node->lhs->stmts;
    if (((node_t *)vec_get(stmts, vec_len(stmts) - 1))->ty != ND_RETURN) {
      printf("  pop rbp\n");
      printf("  ret\n");
    }
    used = 0;
    vstack = 0;
    return;
  }
  else if (node->ty == ND_STMTS) {
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
    used--;
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
    if (node->if_else) {
      gen_asm(node->else_stmt);
    }
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
  else if (node->ty == ND_PARAMS) {
    for (int i = vec_len(node->params) - 1; i >= 0; i--) {
      node_t *param = vec_get(node->params, i);
      gen_asm(param);
      switch (i) {
        case 0:
          printf("  mov rdi, %s\n", regs[used - 1]);
          break;
        case 1:
          printf("  mov rsi, %s\n", regs[used - 1]);
          break;
        case 2:
          printf("  mov rdx, %s\n", regs[used - 1]);
          break;
        case 3:
          printf("  mov rcx, %s\n", regs[used - 1]);
          break;
        case 4:
          printf("  mov r8, %s\n", regs[used - 1]);
          break;
        case 5:
          printf("  mov r9, %s\n", regs[used - 1]);
          break;
        default:
          printf("  push %s\n", regs[used - 1]);
          break;
      }
      used--;
    }
    return;
  }
  else if (node->ty == ND_FUNC_CALL) {
    for (int i = 0; i < used; i++) {
      printf("  push %s\n", regs[i]);
    }
    gen_asm(node->rhs);
    printf("  call _%s\n", node->str);
    int param_size = (int)vec_len(node->rhs->params);
    if (param_size > 5)
      printf("  add rsp, %d\n", (param_size - 6) * 8);
    if (used != 0) {
      printf("  mov %s, rax\n", regs[used]);
    }

    for (int i = 0; i < used; i++) {
      printf("  pop %s\n", regs[i]);
    }
    used++;
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
      case '>':
        printf("  xor r8, r8\n");
        printf("  cmp %s, %s\n", regs[used - 2], regs[used - 1]);
        printf("  cmovng %s, r8\n", regs[used - 2]);
        printf("  mov r8, 1\n");
        printf("  cmovg %s, r8\n", regs[used - 2]);
        used--;
        break;
      case '<':
        printf("  xor r8, r8\n");
        printf("  cmp %s, %s\n", regs[used - 2], regs[used - 1]);
        printf("  cmovnl %s, r8\n", regs[used - 2]);
        printf("  mov r8, 1\n");
        printf("  cmovl %s, r8\n", regs[used - 2]);
        used--;
        break;
      default:
        error("Unknown operator: %d", op);
    }
    return;
  }
}
