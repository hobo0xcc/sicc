#include "sicc.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define REG(n) get_reg(n, ins->size)
#define ARG_REG(n) get_arg_reg(n, ins->size)
#define REG_ORIG(n) get_reg(n, ins->orig_size)

#define POINTER_SIZE 8
#define AX 7
#define DI 8

static const char *regs[] = {"r10", "r11", "rbx", "r12", "r13",
                             "r14", "r15", "rax", "rdi"};
static const char *regs_32[] = {"r10d", "r11d", "ebx",  "r12d",
                                "r13d", "r14d", "r15d", "eax"};
static const char *regs_16[] = {"r10w", "r11w", "bx",   "r12w",
                                "r13w", "r14w", "r15w", "ax"};
static const char *regs_8[] = {"r10b", "r11b", "bl",   "r12b",
                               "r13b", "r14b", "r15b", "al"};
static const char *arg_regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
static const char *arg_regs_32[] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};
static const char *arg_regs_16[] = {"di", "si", "dx", "cx", "r8w", "r9w"};
static const char *arg_regs_8[] = {"dil", "sil", "dl", "cl", "r8b", "r9b"};

static void emit(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stdout, fmt, ap);
  fprintf(stdout, "\n");
  va_end(ap);
}

static const char *ptr_size(ins_t *ins) {
  if (ins->size == 1)
    return "byte ptr";
  else if (ins->size == 2)
    return "word ptr";
  else if (ins->size == 4)
    return "dword ptr";
  else if (ins->size == 8)
    return "qword ptr";
  else
    error("Undefined size: %d op: %d", ins->size, ins->op);
  return NULL;
}

static const char *get_reg(int n, int size) {
  if (size == 1)
    return regs_8[n];
  if (size == 2)
    return regs_16[n];
  if (size == 4)
    return regs_32[n];
  if (size == 8)
    return regs[n];
  error("Undefined size: %d", size);
  return NULL;
}

static const char *get_arg_reg(int n, int size) {
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

static void init_global_var(ir_t *ir, node_t *init) {
  if (init->ty == ND_NUM) {
    emit("  .long %d", init->num);
    return;
  }
  if (init->ty == ND_CHARACTER) {
    emit("  .byte %d", *(init->str));
    return;
  }
  if (init->ty == ND_STRING) {
    vec_push(ir->const_str, init->str);
    int i = vec_len(ir->const_str) - 1;
    emit("  .quad .LC%d", i);
    return;
  }
  if (init->ty == ND_INITIALIZER) {
    int len = vec_len(init->initializer);
    for (int i = 0; i < len; i++) {
      init_global_var(ir, vec_get(init->initializer, i));
    }
    return;
  }
  if (init->ty == ND_IDENT) {
    gvar_t *gvar = map_get(ir->gvars, init->str);
    if (!gvar) {
      error("%s is not defined as global variable", init->str);
    }
    
    if (!gvar->is_null) {
      error("Cannot initialize global var with %s", init->str);
    }
    init_global_var(ir, gvar->init);
    return;
  }
  
  error("Cannot initialize global var with %s", init->ty);
}

void gen_asm(ir_t *ir) {
  int len = vec_len(ir->code);
  // Number of global functions
  int ngfuncs = vec_len(ir->gfuncs);
  emit(".intel_syntax noprefix");
  // Globalize functions
  for (int i = 0; i < ngfuncs; i++) {
    emit(".global _%s", vec_get(ir->gfuncs, i));
  }

  // Number of global variables
  int ngvars = map_len(ir->gvars);
  emit(".section __DATA,_data");
  for (int i = 0; i < ngvars; i++) {
    gvar_t *gvar = vec_get(ir->gvars->items, i);
    if (gvar->external)
      emit(".global _%s", gvar->name);
    // Definition of uninitialized global variable
    if (gvar->is_null) {
      // http://web.mit.edu/gnu/doc/html/as_7.html#SEC74
      emit("  .comm _%s, %d", gvar->name, gvar->size);
    } else {
      emit("_%s: ", gvar->name);
      init_global_var(ir, gvar->init);
    }
  }

  // Number of constant strings
  int nconsts = vec_len(ir->const_str);
  emit(".section __TEXT,__cstring");
  for (int i = 0; i < nconsts; i++) {
    char *s = vec_get(ir->const_str, i);
    emit(".LC%d:\n  .asciz \"%s\"", i, s);
  }

  emit("\n.section __TEXT,__text");
  for (int pc = 0; pc < len; pc++) {
    ins_t *ins = vec_get(ir->code, pc);
    int lhs = ins->lhs;
    int rhs = ins->rhs;

    switch (ins->op) {
    case IR_MOV_IMM:
      emit("  mov %s, %d", REG(lhs), rhs);
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
      emit("  add %s, %s", REG(lhs), REG(rhs));
      break;
    case IR_SUB:
      emit("  sub %s, %s", REG(lhs), REG(rhs));
      break;
    case IR_MUL:
      emit("  push rdx");
      emit("  mov rax, %s", regs[lhs]);
      emit("  mul %s", regs[rhs]);
      emit("  add rax, rdx");
      emit("  mov %s, %s", REG(lhs), REG(AX));
      emit("  pop rdx");
      break;
    case IR_DIV:
      emit("  push rdx");
      emit("  mov rax, %s", regs[lhs]);
      emit("  cqo");
      emit("  div %s", regs[rhs]);
      emit("  mov %s, %s", REG(lhs), REG(AX));
      emit("  pop rdx");
      break;
    case IR_GREAT:
      emit("  cmp %s, %s", REG(lhs), REG(rhs));
      emit("  setg al");
      emit("  movzx %s, al", REG(lhs));
      emit("  mov al, 0");
      break;
    case IR_LESS:
      emit("  cmp %s, %s", REG(lhs), REG(rhs));
      emit("  setl al");
      emit("  movzx %s, al", REG(lhs));
      emit("  mov al, 0");
      break;
    case IR_NOT:
      emit("  cmp %s, 0", regs[lhs]);
      emit("  sete al");
      emit("  movzx %s, al", regs[lhs]);
      emit("  mov al, 0");
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
      emit(".L%d:", lhs);
      break;
    case IR_LABEL_BB:
      emit(".LBB%d: ", lhs);
      break;
    case IR_LABEL_BBEND:
      emit(".LBB_END%d: ", lhs);
      break;
    case IR_LABEL_BBSTART:
      emit(".LBB_START%d: ", lhs);
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
    case IR_JTRUE:
      emit("  test %s, %s", regs[lhs], regs[lhs]);
      emit("  jnz .L%d", rhs);
      break;
    case IR_JTRUE_BB:
      emit("  test %s, %s", regs[lhs], regs[lhs]);
      emit("  jnz .LBB%d", rhs);
      break;
    case IR_JTRUE_BBEND:
      emit("  test %s, %s", regs[lhs], regs[lhs]);
      emit("  jnz .LBB_END%d", rhs);
      break;
    case IR_JZERO:
      emit("  test %s, %s", regs[lhs], regs[lhs]);
      emit("  jz .L%d", rhs);
      break;
    case IR_JZERO_BB:
      emit("  test %s, %s", regs[lhs], regs[lhs]);
      emit("  jz .LBB%d", rhs);
      break;
    case IR_JZERO_BBEND:
      emit("  test %s, %s", regs[lhs], regs[lhs]);
      emit("  jz .LBB_END%d", rhs);
      break;
    case IR_JMP:
      emit("  jmp .L%d", lhs);
      break;
    case IR_JMP_BB:
      emit("  jmp .LBB%d", lhs);
      break;
    case IR_JMP_BBEND:
      emit("  jmp .LBB_END%d", lhs);
      break;
    case IR_JMP_BBSTART:
      emit("  jmp .LBB_START%d", lhs);
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
    case IR_PTR_CAST:
      emit("  mov %s, %s", regs[AX], regs[lhs]);
      // emit("  cdqe");
      emit("  lea %s, [0+rax*%d]", regs[lhs], ins->size);
      break;
    case IR_EQ:
      emit("  cmp %s, %s", REG(lhs), REG(rhs));
      emit("  sete al");
      emit("  movzx %s, al", REG(lhs));
      emit("  mov al, 0");
      break;
    case IR_NEQ:
      emit("  cmp %s, %s", REG(lhs), REG(rhs));
      emit("  setne al");
      emit("  movzx %s, al", REG(lhs));
      emit("  mov al, 0");
      break;
    case IR_LOAD_ADDR_VAR:
      emit("  lea %s, [rbp%+d]", regs[lhs], -rhs);
      break;
    case IR_PUSH:
      emit("  push %s", regs[lhs]);
      break;
    case IR_POP:
      emit("  pop %s", regs[lhs]);
      break;
    case IR_LOAD_GVAR:
      emit("  mov %s, %s [rip+_%s]", REG(lhs), ptr_size(ins), ins->name);
      break;
    case IR_LOAD_ADDR_GVAR:
      emit("  lea %s, [rip+_%s]", regs[lhs], ins->name);
      break;
    case IR_ADD_IMM:
      emit("  add %s, %d", REG(lhs), rhs);
      break;
    case IR_SUB_IMM:
      emit("  sub %s, %d", REG(lhs), rhs);
      break;
    case IR_MOV:
      emit("  mov %s, %s", REG(lhs), REG(rhs));
      break;
    case IR_LOGAND:
      emit("  and %s, %s", REG(lhs), REG(rhs));
      emit("  setnz al");
      emit("  movzx %s, al", REG(lhs));
      emit("  mov al, 0");
      break;
    case IR_LOGOR:
      emit("  or %s, %s", REG(lhs), REG(rhs));
      emit("  setnz al");
      emit("  movzx %s, al", REG(lhs));
      emit("  mov al, 0");
      break;
    case IR_CAST:
      if (ins->size < rhs) {
        emit("  movzx %s, %s", regs[lhs], REG(lhs));
      }
      break;
    default:
      error("Unknown IR type: %d", ins->op);
    }
  }
}
