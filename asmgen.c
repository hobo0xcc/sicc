#include "sicc.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

static const char *regs[] = { "r10", "r11", "rbx", "r12", "r13", "r14", "r15" };
static const char *arg_regs[] = { "rdi", "rsi", "rdx", "rcx", "r8", "r9" };

static void emit(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stdout, fmt, ap);
  fprintf(stdout, "\n");
  va_end(ap);
}

void gen_asm(ir_t *ir)
{
  int len = vec_len(ir->code);
  int ngfuncs = vec_len(ir->gfuncs);
  emit(".intel_syntax noprefix");
  for (int i = 0; i < ngfuncs; i++) {
    emit(".global _%s", vec_get(ir->gfuncs, i));
  }

  for (int pc = 0; pc < len; pc++) {
    ins_t *ins = vec_get(ir->code, pc);
    int lhs = ins->lhs;
    int rhs = ins->rhs;

    switch (ins->op) {
      case IR_MOV_IMM:
        emit("  mov %s, %d", regs[lhs], rhs);
        break;
      case IR_MOV_RETVAL:
        emit("  mov %s, rax", regs[lhs]);
        break;
      case IR_STORE_ARG:
        if (lhs < 6)
          emit("  mov %s, %s", arg_regs[lhs], regs[rhs]);
        else
          emit("  push %s", regs[rhs]);
        break;
      case IR_LOAD_ARG:
        if (rhs < 6)
          emit("  mov [rbp%+d], %s", -lhs, arg_regs[rhs]);
        break;
      case IR_ADD:
        emit("  add %s, %s", regs[lhs], regs[rhs]);
        break;
      case IR_SUB:
        emit("  sub %s, %s", regs[lhs], regs[rhs]);
        break;
      case IR_MUL:
        emit("  mov rax, %s", regs[lhs]);
        emit("  mul %s", regs[rhs]);
        emit("  add rax, rdx");
        emit("  mov %s, rax", regs[lhs]);
        break;
      case IR_DIV:
        emit("  mov rax, %s", regs[lhs]);
        emit("  cqo");
        emit("  div %s", regs[rhs]);
        emit("  mov %s, rax", regs[lhs]);
        break;
      case IR_GREAT:
        emit("  cmp %s, %s", regs[lhs], regs[rhs]);
        emit("  setl al");
        emit("  movzx %s, al", regs[lhs]);
        break;
      case IR_LESS:
        emit("  cmp %s, %s", regs[lhs], regs[rhs]);
        emit("  setg al");
        emit("  movzx %s, al", regs[lhs]);
        break;
      case IR_STORE:
        emit("  mov [rbp%+d], %s", -lhs, regs[rhs]);
        break;
      case IR_LOAD:
        emit("  mov %s, [rbp%+d]", regs[lhs], -rhs);
        break;
      case IR_CALL:
        emit("  call _%s", ins->name);
        break;
      case IR_FUNC:
        emit("_%s:", ins->name);
        emit("  push rbp");
        emit("  mov rbp, rsp");
        break;
      case IR_LABEL:
        emit("  .L%d:", lhs);
        break;
      case IR_ALLOC:
        emit("  sub rsp, %d", lhs);
        break;
      case IR_FREE:
        emit("  add rsp, %d", lhs);
        break;
      case IR_RET:
        emit("  pop rbp");
        emit("  mov rax, %s", regs[lhs]);
        emit("  ret");
        break;
      case IR_SAVE_REG:
        emit("  push %s", regs[lhs]);
        break;
      case IR_REST_REG:
        emit("  pop %s", regs[lhs]);
        break;
      default:
        error("Unknown IR type: %d", ins->op);
    }
  }
}
