#include "sicc.h"

#include <stdlib.h>

static int nreg = 0;
static int nlabel = 0;
static int stack_size = 0;
static int cur_stack = 0;

ir_t *new_ir()
{
  ir_t *ir = calloc(1, sizeof(ir_t));
  ir->code = new_vec();
  ir->vars = new_map();
  ir->gfuncs = new_vec();
  return ir;
}

static ins_t *emit(ir_t *ir, int op, int lhs, int rhs)
{
  ins_t *ins = calloc(1, sizeof(ins_t));
  ins->op = op;
  ins->lhs = lhs;
  ins->rhs = rhs;
  vec_push(ir->code, ins);
  return ins;
}

static int alloc_stack(int size)
{
  cur_stack += size;
  if (stack_size < cur_stack)
    stack_size = cur_stack;
  return cur_stack;
}

static void gen_stmt(ir_t *ir, node_t *node);
static int gen_expr(ir_t *ir, node_t *node);
static int gen_assign(ir_t *ir, node_t *node, int left, int right);

static void gen_stmt(ir_t *ir, node_t *node)
{
  if (node->ty == ND_FUNCS) {
    int len = vec_len(node->funcs);
    for (int i = 0; i < len; i++) {
      node_t *func = vec_get(node->funcs, i);
      gen_stmt(ir, func);
    }
    return;
  }
  if (node->ty == ND_FUNC) {
    ins_t *func = emit(ir, IR_FUNC, -1, -1);
    vec_push(ir->gfuncs, node->str);
    func->name = node->str;
    ins_t *stack_alloc = emit(ir, IR_ALLOC, 0, -1);
    gen_stmt(ir, node->rhs);
    gen_stmt(ir, node->lhs);
    stack_alloc->lhs = stack_size;
    stack_size = 0;
    cur_stack = 0;
    nreg = 0;
    return;
  }
  if (node->ty == ND_ARGS) {
    int len = vec_len(node->args); // Arguments length
    int arg_stack = 16;
    for (int i = 0; i < len; i++) {
      node_t *arg = vec_get(node->args, i);
      if (i > 5) {
        int offset = arg_stack;
        map_put(ir->vars, arg->str, (void *)(intptr_t)offset);
        emit(ir, IR_LOAD_ARG, -offset, i);
      }
      else {
        int offset = alloc_stack(8);
        map_put(ir->vars, arg->str, (void *)(intptr_t)offset);
        emit(ir, IR_LOAD_ARG, offset, i);
      }
    }
    return;
  }
  if (node->ty == ND_STMTS) {
    int len = vec_len(node->stmts);
    for (int i = 0; i < len; i++) {
      node_t *stmt = vec_get(node->stmts, i);
      gen_ir(ir, stmt);
    }
    return;
  }
  if (node->ty == ND_RETURN) {
    int r = gen_ir(ir, node->lhs);
    emit(ir, IR_FREE, stack_size, -1);
    emit(ir, IR_RET, r, -1);
    return;
  }
  
  error("Not implemented yet: %d", node->ty);
}

static int gen_assign(ir_t *ir, node_t *node, int left, int right)
{
  if (left == -1) {
    int offset = alloc_stack(8);
    map_put(ir->vars,
        node->lhs->str,
        (void *)(intptr_t)offset);
    emit(ir, IR_STORE, offset, right);
    return nreg;
  }
  else {
    int offset = (int)(intptr_t)map_get(ir->vars, node->lhs->str);
    emit(ir, IR_STORE, offset, right);
    nreg--;
    return nreg;
  }
}

static int gen_expr(ir_t *ir, node_t *node)
{
  int left = gen_ir(ir, node->lhs);
  int op = node->op;
  int right = gen_ir(ir, node->rhs);

  switch (op) {
    case '+':
      emit(ir, IR_ADD, left, right);
      break;
    case '-':
      emit(ir, IR_SUB, left, right);
      break;
    case '*':
      emit(ir, IR_MUL, left, right);
      break;
    case '/':
      emit(ir, IR_DIV, left, right);
      break;
    case '>':
      emit(ir, IR_GREAT, left, right);
      break;
    case '<':
      emit(ir, IR_LESS, left, right);
      break;
    case '=':
      left = gen_assign(ir, node, left, right);
      break;
    default:
      error("Unknown operator: %d", op);
  }
  nreg--;
  return left;
}

int gen_ir(ir_t *ir, node_t *node)
{
  if (node->ty == ND_EXPR) {
    return gen_expr(ir, node);
  }
  if (node->ty == ND_NUM) {
    emit(ir, IR_MOV_IMM, nreg++, node->num);
    return nreg - 1;
  }
  if (node->ty == ND_IDENT) {
    if (map_find(ir->vars, node->str)) {
      int offset = (int)(intptr_t)map_get(ir->vars, node->str);
      emit(ir, IR_LOAD, nreg++, offset);
      return nreg - 1;
    }
    else
      return -1;
  }
  if (node->ty == ND_FUNC_CALL) {
    for (int i = 0; i < nreg; i++)
      emit(ir, IR_SAVE_REG, i, -1);
    int saved_reg = nreg;

    gen_ir(ir, node->rhs);
    ins_t *ins = emit(ir, IR_CALL, -1, -1);
    ins->name = node->str;
    for (int i = 0; i < saved_reg; i++)
      emit(ir, IR_REST_REG, i, -1);

    emit(ir, IR_MOV_RETVAL, nreg++, -1);
    return nreg - 1;
  }
  if (node->ty == ND_PARAMS) {
    int len = vec_len(node->params);
    for (int i = len - 1, a_reg = 0; i >= 0; i--, a_reg++) {
      int r = gen_ir(ir, vec_get(node->params, i));
      emit(ir, IR_STORE_ARG, a_reg, r);
      nreg--;
    }
    return -1;
  }

  gen_stmt(ir, node);
  return -1;
}

void print_ir(ir_t *ir)
{
  int len = vec_len(ir->code);
  for (int i = 0; i < len; i++) {
    ins_t *ins = vec_get(ir->code, i);
    switch (ins->op) {
      case IR_MOV_IMM:
        printf("  mov_imm r%d, %d\n", ins->lhs, ins->rhs);
        break;
      case IR_MOV_RETVAL:
        printf("  mov_retval r%d, retval\n", ins->lhs);
        break;
      case IR_STORE_ARG:
        printf("  store_arg a%d, r%d\n", ins->lhs, ins->rhs);
        break;
      case IR_LOAD_ARG:
        printf("  load_arg v%d, a%d\n", ins->lhs, ins->rhs);
        break;
      case IR_ADD:
        printf("  add r%d, r%d\n", ins->lhs, ins->rhs);
        break;
      case IR_SUB:
        printf("  sub r%d, r%d\n", ins->lhs, ins->rhs);
        break;
      case IR_MUL:
        printf("  mul r%d, r%d\n", ins->lhs, ins->rhs);
        break;
      case IR_DIV:
        printf("  div r%d, r%d\n", ins->lhs, ins->rhs);
        break;
      case IR_GREAT:
        printf("  great r%d, r%d", ins->lhs, ins->rhs);
        break;
      case IR_LESS:
        printf("  less r%d, r%d\n", ins->lhs, ins->rhs);
        break;
      case IR_STORE:
        printf("  store v%d, r%d\n", ins->lhs, ins->rhs);
        break;
      case IR_LOAD:
        printf("  load r%d, v%d\n", ins->lhs, ins->rhs);
        break;
      case IR_CALL:
        printf("  call %s\n", ins->name);
        break;
      case IR_FUNC:
        printf("func %s:\n", ins->name);
        break;
      case IR_LABEL:
        printf("  .L%s:\n", ins->name);
        break;
      case IR_ALLOC:
        printf("  alloc %d\n", ins->lhs);
        break;
      case IR_FREE:
        printf("  free %d\n", ins->lhs);
        break;
      case IR_RET:
        printf("  ret r%d\n", ins->lhs);
        break;
      case IR_SAVE_REG:
        printf("  save_reg r%d\n", ins->lhs);
        break;
      case IR_REST_REG:
        printf("  rest_reg r%d\n", ins->lhs);
        break;
      default:
        error("Unknown operator: %d", ins->op);
    }
  }
}
