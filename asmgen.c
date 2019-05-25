#include "sicc.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define REG(n) get_reg(n, ins->size)
#define ARG_REG(n) get_arg_reg(n, ins->size)

static const char *regs[] = { "r10", "r11", "rbx", "r12", "r13", "r14", "r15" };
static const char *regs_32[] = { "r10d", "r11d", "ebx", "r12d", "r13d", "r14d", "r15d" };
static const char *regs_16[] = { "r10w", "r11w", "bx", "r12w", "r13w", "r14w", "r15w" };
static const char *regs_8[] = { "r10b", "r11b", "bl", "r12b", "r13b", "r14b", "r15b" };
static const char *arg_regs[] = { "rdi", "rsi", "rdx", "rcx", "r8", "r9" };
static const char *arg_regs_32[] = { "edi", "esi", "edx", "ecx", "r8d", "r9d" };
static const char *arg_regs_16[] = { "di", "si", "dx", "cx", "r8w", "r9w" };
static const char *arg_regs_8[] = { "bh", "dh", "dl", "cl", "r8l", "r9l" };

static void emit(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stdout, fmt, ap);
  fprintf(stdout, "\n");
  va_end(ap);
}

static const char *ptr_size(ins_t *ins)
{
  if (ins->size == 1)
    return "byte ptr";
  else if (ins->size == 2)
    return "word ptr";
  else if (ins->size == 4)
    return "dword ptr";
  else if (ins->size == 8)
    return "qword ptr";
  else
    error("Too big size: %d op: %d", ins->size, ins->op);
  return NULL;
}

static const char *get_reg(int n, int size)
{
  if (size == 1)
    return regs_8[n];
  if (size == 2)
    return regs_16[n];
  if (size == 4)
    return regs_32[n];
  if (size == 8)
    return regs[n];
  error("Too big size: %d", size);
  return NULL;
}

static const char *get_arg_reg(int n, int size)
{
  if (size == 1)
    return arg_regs_8[n];
  if (size == 2)
    return arg_regs_16[n];
  if (size == 4)
    return arg_regs_32[n];
  if (size == 8)
    return arg_regs[n];
  error("Variable size is only 1, 2, 4, 8 currently: %d", size);
  return NULL;
}

void gen_asm(ir_t *ir)
{
  int len = vec_len(ir->code);
  int ngfuncs = vec_len(ir->gfuncs);
  emit(".intel_syntax noprefix");
  for (int i = 0; i < ngfuncs; i++) {
    emit(".global _%s", vec_get(ir->gfuncs, i));
  }

  int nconsts = vec_len(ir->const_str);
  for (int i = 0; i < nconsts; i++) {
    char *s = vec_get(ir->const_str, i);
    emit(".LC%d:\n  .string \"%s\"", i, s);
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
          emit("  mov %s, %s", ARG_REG(lhs), REG(rhs));
        else
          emit("  push %s", regs[rhs]);
        break;
      case IR_LOAD_ARG:
        if (rhs < 6)
          emit("  mov %s [rbp%+d], %s", ptr_size(ins), -lhs, ARG_REG(rhs));
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
        emit("  setg al");
        emit("  movzx %s, al", regs[lhs]);
        break;
      case IR_LESS:
        emit("  cmp %s, %s", regs[lhs], regs[rhs]);
        emit("  setl al");
        emit("  movzx %s, al", regs[lhs]);
        break;
      case IR_STORE:
        emit("  mov %s [%s], %s", ptr_size(ins), regs[lhs], REG(rhs));
        break;
      case IR_LOAD:
        emit("  mov %s, %s [%s]", REG(lhs), ptr_size(ins), regs[rhs]);
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
        emit("  mov rax, %s", regs[lhs]);
        // emit("  leave");
        // emit("  ret");
        break;
      case IR_SAVE_REG:
        emit("  push %s", regs[lhs]);
        break;
      case IR_REST_REG:
        emit("  pop %s", regs[lhs]);
        break;
      case IR_JTRUE:
        emit("  test %s, %s", regs[lhs], regs[lhs]);
        emit("  jnz .L%d", rhs);
        break;
      case IR_JMP:
        emit("  jmp .L%d", lhs);
        break;
      case IR_STORE_VAR:
        emit("  mov %s [rbp%+d], %s", ptr_size(ins), -lhs, REG(rhs));
        break;
      case IR_LOAD_VAR:
        emit("  mov %s, %s [rbp%+d]", REG(lhs), ptr_size(ins), -rhs);
        break;
      case IR_LEAVE:
        emit("  leave");
        emit("  ret");
        break;
      case IR_LOAD_CONST:
        emit("  lea %s, [rip+.LC%d]", REG(lhs), rhs);
        break;
      default:
        error("Unknown IR type: %d", ins->op);
    }
  }
}
