#include "sicc.h"

#include <stdlib.h>

static int nreg = 0;
static int nlabel = 0;
static int stack_size = 0;
static int cur_stack = 0;

ir_t *new_ir() {
  ir_t *ir = calloc(1, sizeof(ir_t));
  ir->code = new_vec();
  ir->gvars = new_map();
  ir->vars = new_map();
  ir->gfuncs = new_vec();
  ir->const_str = new_vec();
  return ir;
}

var_t *new_var(int offset, int size) {
  var_t *var = calloc(1, sizeof(var_t));
  var->offset = offset;
  var->size = size;
  return var;
}

gvar_t *new_gvar(char *name, int size) {
  gvar_t *gvar = calloc(1, sizeof(gvar_t));
  gvar->name = name;
  gvar->size = size;
  return gvar;
}

static ins_t *emit(ir_t *ir, int op, int lhs, int rhs, int size) {
  ins_t *ins = calloc(1, sizeof(ins_t));
  ins->op = op;
  ins->lhs = lhs;
  ins->rhs = rhs;
  ins->size = size;
  vec_push(ir->code, ins);
  return ins;
}

static int alloc_stack(int size) {
  cur_stack += size;
  if (stack_size < cur_stack)
    stack_size = cur_stack;
  return cur_stack;
}

static int free_stack(int size) {
  cur_stack -= size;
  return cur_stack;
}

static var_t *gen_initializer(ir_t *ir, node_t *node, type_t *array_ty,
                              int offset);
static void gen_stmt(ir_t *ir, node_t *node);
static int gen_expr(ir_t *ir, node_t *node);
static int gen_assign(ir_t *ir, node_t *node, int left, int right);

static var_t *gen_initializer(ir_t *ir, node_t *node, type_t *array_ty,
                              int offset) {
  int elem_size = array_ty->size_deref;
  int elem_offset = offset;
  int elem_len = array_ty->array_size;
  for (int i = 0; i < elem_len; i++, elem_offset -= elem_size) {
    node_t *expr = vec_get(node->initializer, i);
    int r = gen_ir(ir, expr);
    emit(ir, IR_STORE_VAR, elem_offset, r, elem_size);
    nreg--;
  }

  var_t *var = new_var(offset, array_ty->size_deref);
  return var;
}

static void gen_stmt(ir_t *ir, node_t *node) {
  if (node->ty == ND_FUNCS) {
    int len = vec_len(node->funcs);
    for (int i = 0; i < len; i++) {
      node_t *func = vec_get(node->funcs, i);
      gen_stmt(ir, func);
    }
    return;
  }
  if (node->ty == ND_EXTERNAL) {
    int len = vec_len(node->decl_list);
    for (int i = 0; i < len; i++) {
      node_t *decl = vec_get(node->decl_list, i);
      gen_stmt(ir, decl);
    }

    len = vec_len(node->funcs);
    for (int i = 0; i < len; i++) {
      node_t *func = vec_get(node->funcs, i);
      gen_stmt(ir, func);
    }

    return;
  }
  if (node->ty == ND_FUNC) {
    ins_t *func = emit(ir, IR_FUNC, -1, -1, -1);
    vec_push(ir->gfuncs, node->str);
    func->name = node->str;
    ins_t *stack_alloc = emit(ir, IR_ALLOC, 0, -1, -1);
#ifdef __APPLE__
    alloc_stack(4);
    emit(ir, IR_MOV_IMM, nreg, 0, 4);
    emit(ir, IR_STORE_VAR, 4, nreg, 4);
#endif
    gen_stmt(ir, node->rhs);
    gen_stmt(ir, node->lhs);
    if (stack_size % 16 != 0)
      stack_size += 16 - (stack_size % 16);
    stack_alloc->lhs = stack_size;
    stack_size = 0;
    cur_stack = 0;
    nreg = 0;
    free(ir->vars);
    ir->vars = new_map();
    return;
  }
  if (node->ty == ND_ARGS) {
    int len = vec_len(node->args); // Arguments length
    int arg_stack = -16;
    for (int i = 0; i < len; i++) {
      node_t *arg = vec_get(node->args, i);
      if (i > 5) {
        int offset = arg_stack;
        var_t *var = new_var(offset, arg->type->size);
        map_put(ir->vars, arg->str, var);
        ins_t *ins = emit(ir, IR_LOAD_ARG, offset, i, arg->type->size);
        arg_stack -= arg->type->size;
      } else {
        int offset = alloc_stack(arg->type->size);
        var_t *var = new_var(offset, arg->type->size);
        map_put(ir->vars, arg->str, var);
        emit(ir, IR_LOAD_ARG, offset, i, arg->type->size);
      }
    }
    return;
  }
  if (node->ty == ND_STMTS) {
    int len = vec_len(node->stmts);
    int init_stack = cur_stack;
    int init_nvar = map_len(ir->vars) - 1;
    for (int i = 0; i < len; i++) {
      node_t *stmt = vec_get(node->stmts, i);
      gen_ir(ir, stmt);
    }

    int nvar = map_len(ir->vars) - 1;
    for (int i = init_nvar; i < nvar; i++) {
      map_pop(ir->vars);
    }

    return;
  }
  if (node->ty == ND_RETURN) {
    int r = gen_ir(ir, node->lhs);
    emit(ir, IR_FREE, stack_size, -1, -1);
    emit(ir, IR_RET, r, -1, -1);
    emit(ir, IR_LEAVE, -1, -1, -1);
    nreg--;
    return;
  }
  if (node->ty == ND_IF) {
    int r = gen_ir(ir, node->rhs);
    emit(ir, IR_JTRUE, r, nlabel++, -1);
    nreg--;
    emit(ir, IR_JMP, nlabel++, -1, -1);
    emit(ir, IR_LABEL, nlabel - 2, -1, -1);
    gen_ir(ir, node->lhs);
    emit(ir, IR_LABEL, nlabel - 1, -1, -1);
    return;
  }
  if (node->ty == ND_IF_ELSE) {
    int r = gen_ir(ir, node->rhs);
    emit(ir, IR_JTRUE, r, nlabel++, -1);
    nreg--;
    emit(ir, IR_JMP, nlabel++, -1, -1);
    emit(ir, IR_LABEL, nlabel - 2, -1, -1);
    gen_ir(ir, node->lhs);
    emit(ir, IR_LABEL, nlabel - 1, -1, -1);
    gen_ir(ir, node->else_stmt);
    return;
  }
  if (node->ty == ND_WHILE) {
    int eval = nlabel++;
    int prog = nlabel++;
    emit(ir, IR_JMP, eval, -1, -1);
    emit(ir, IR_LABEL, prog, -1, -1);
    gen_ir(ir, node->lhs);
    emit(ir, IR_LABEL, eval, -1, -1);
    int r = gen_ir(ir, node->rhs);
    emit(ir, IR_JTRUE, r, prog, -1);
    nreg--;
    return;
  }
  if (node->ty == ND_VAR_DEF) {
    if (map_find(ir->vars, node->str))
      error("%s is already defined", node->str);
    int offset = alloc_stack(node->type->size);
    int r;
    if (node->lhs->ty == ND_INITIALIZER) {
      if (node->type->ty != TY_ARRAY) {
        error("Initializer is only to use to an array");
      } else {
        var_t *var = gen_initializer(ir, node->lhs, node->type, offset);
        map_put(ir->vars, node->str, var);
      }
      return;
    } else {
      r = gen_ir(ir, node->lhs);
    }

    emit(ir, IR_STORE_VAR, offset, r, node->type->size);
    var_t *var;
    if (node->type->ty == TY_ARRAY)
      var = new_var(offset, node->type->size_deref);
    else
      var = new_var(offset, node->type->size);
    map_put(ir->vars, node->str, var);
    nreg--;
    return;
  }
  if (node->ty == ND_VAR_DECL) {
    if (map_find(ir->vars, node->str))
      error("%s is already defined", node->str);
    int offset = alloc_stack(node->type->size);
    var_t *var = new_var(offset, node->type->size);
    map_put(ir->vars, node->str, var);
    return;
  }
  if (node->ty == ND_VAR_DECL_LIST) {
    for (int i = 0, len = vec_len(node->vars); i < len; i++) {
      gen_ir(ir, vec_get(node->vars, i));
    }
    return;
  }
  if (node->ty == ND_EXT_VAR_DEF) {
    if (map_find(ir->gvars, node->str))
      error("%s is already defined", node->str);
    if (node->lhs->ty == ND_INITIALIZER) {
      if (node->type->ty != TY_ARRAY) {
        error("Initializer is only to use to an array");
      } else {
        error("Global array initializer is not implemented yet");
      }
      return;
    }

    gvar_t *gvar = new_gvar(node->str, node->type->size);
    gvar->init = node->lhs;
    gvar->is_null = 0;
    map_put(ir->gvars, node->str, gvar);

    return;
  }
  if (node->ty == ND_EXT_VAR_DECL) {
    if (map_find(ir->gvars, node->str))
      error("%s is already defined", node->str);
    gvar_t *gvar = new_gvar(node->str, node->type->size);
    gvar->is_null = 1;
    gvar->init = NULL;
    map_put(ir->gvars, node->str, gvar);
    return;
  }

  error("Not implemented yet: %d", node->ty);
}

static int gen_assign(ir_t *ir, node_t *node, int left, int right) {
  if (left == -1) {
    error("%s is not declared", node->lhs->str);
    return -1;
  } else {
    if (node->lhs->ty == ND_DEREF_LVAL ||
        node->lhs->ty == ND_DEREF_INDEX_LVAL) {
      emit(ir, IR_STORE, left, right, node->lhs->type->size);
      nreg--;
      return nreg;
    } else if (node->lhs->ty == ND_IDENT_LVAL) {
      emit(ir, IR_STORE, left, right, node->lhs->type->size);
      nreg--;
      return nreg;
    } else {
      error("Invalid lvalue type");
      return -1;
    }
  }
}

static int gen_expr(ir_t *ir, node_t *node) {
  int left;
  int op;
  int right;
  int size;

  left = gen_ir(ir, node->lhs);
  op = node->op;
  right = gen_ir(ir, node->rhs);
  if (node->type->ty == TY_PTR || node->type->ty == TY_ARRAY) {
    emit(ir, IR_PTR_CAST, right, -1, node->type->size_deref);
    size = 8;
  } else {
    size = node->type->size;
  }

  switch (op) {
  case '+':
    emit(ir, IR_ADD, left, right, size);
    break;
  case '-':
    emit(ir, IR_SUB, left, right, size);
    break;
  case '*':
    emit(ir, IR_MUL, left, right, size);
    break;
  case '/':
    emit(ir, IR_DIV, left, right, size);
    break;
  case '>':
    emit(ir, IR_GREAT, left, right, size);
    break;
  case '<':
    emit(ir, IR_LESS, left, right, size);
    break;
  case '=':
    left = gen_assign(ir, node, left, right);
    break;
  case OP_PLUS_ASSIGN:
    emit(ir, IR_ADD, left, right, size);
    left = gen_assign(ir, node, left, left);
    break;
  case OP_MINUS_ASSIGN:
    emit(ir, IR_SUB, left, right, size);
    left = gen_assign(ir, node, left, left);
    break;
  case OP_EQUAL:
    emit(ir, IR_EQ, left, right, size);
    break;
  case OP_NOT_EQUAL:
    emit(ir, IR_NEQ, left, right, size);
    break;
  default:
    error("Unknown operator: %d", op);
  }
  nreg--;
  return left;
}

int gen_ir(ir_t *ir, node_t *node) {
  if (node->ty == ND_EXPR) {
    return gen_expr(ir, node);
  }
  if (node->ty == ND_NUM) {
    emit(ir, IR_MOV_IMM, nreg++, node->num, node->type->size);
    return nreg - 1;
  }
  if (node->ty == ND_IDENT || node->ty == ND_IDENT_LVAL) {
    if (map_find(ir->vars, node->str)) {
      var_t *var = (var_t *)map_get(ir->vars, node->str);
      if (node->type->ty == TY_ARRAY)
        emit(ir, IR_LOAD_ADDR_VAR, nreg++, var->offset, -1);
      else if (node->ty == ND_IDENT)
        emit(ir, IR_LOAD_VAR, nreg++, var->offset, var->size);
      else // ND_IDENT_LVAL
        emit(ir, IR_LOAD_ADDR_VAR, nreg++, var->offset, var->size);
      return nreg - 1;
    } else if (map_find(ir->gvars, node->str)) {
      gvar_t *gvar = (gvar_t *)map_get(ir->gvars, node->str);
      if (node->ty == ND_IDENT) {
        ins_t *ins = emit(ir, IR_LOAD_GVAR, nreg++, -1, gvar->size);
        ins->name = gvar->name;
      } else { // ND_IDENT_LVAL
        ins_t *ins = emit(ir, IR_LOAD_ADDR_GVAR, nreg++, -1, gvar->size);
        ins->name = gvar->name;
      }
      return nreg - 1;
    } else
      error("Undefined variable: %s", node->str);
  }
  if (node->ty == ND_DEREF || node->ty == ND_DEREF_LVAL) {
    int r = gen_ir(ir, node->lhs);
    if (node->ty == ND_DEREF) {
      emit(ir, IR_LOAD, r, r, node->type->size);
    }
    return r;
  }
  if (node->ty == ND_DEREF_INDEX || node->ty == ND_DEREF_INDEX_LVAL) {
    int left = gen_ir(ir, node->lhs);
    int right = gen_ir(ir, node->rhs);
    int size;
    int is_left_ptr = 1;
    if ((size = node->lhs->type->size_deref) == 0) {
      size = node->rhs->type->size_deref;
      is_left_ptr = 0;
    }
    emit(ir, IR_PTR_CAST, (is_left_ptr ? right : left), -1, size);
    emit(ir, IR_ADD, left, right, 8);
    if (node->ty == ND_DEREF_INDEX)
      emit(ir, IR_LOAD, left, left, size);
    nreg--;
    return left;
  }
  if (node->ty == ND_REF) {
    int r = gen_ir(ir, node->lhs);
    return r;
  }
  if (node->ty == ND_FUNC_CALL) {
    gen_ir(ir, node->rhs);
    for (int i = 0; i < nreg; i++) {
      emit(ir, IR_PUSH, i, -1, -1);
    }
    int saved_regs = nreg;
    ins_t *ins = emit(ir, IR_CALL, -1, -1, -1);
    ins->name = node->str;
    if (node->flag->should_save)
      emit(ir, IR_MOV_RETVAL, nreg++, -1, -1);
    for (int i = 0; i < saved_regs; i++) {
      emit(ir, IR_POP, i, -1, -1);
    }
    return nreg - 1;
  }
  if (node->ty == ND_PARAMS) {
    int len = vec_len(node->params);
    for (int i = len - 1; i >= 0; i--) {
      node_t *param = vec_get(node->params, i);
      int r = gen_ir(ir, param);
      emit(ir, IR_STORE_ARG, i, r, param->type->size);
      nreg--;
    }
    return -1;
  }
  if (node->ty == ND_STRING) {
    vec_push(ir->const_str, node->str);
    int i = vec_len(ir->const_str) - 1;
    emit(ir, IR_LOAD_CONST, nreg++, i, node->type->size);
    return nreg - 1;
  }

  if (node->ty == ND_CHARACTER) {
    emit(ir, IR_MOV_IMM, nreg++, node->num, node->type->size);
    return nreg - 1;
  }

  if (node->ty == ND_SIZEOF) {
    emit(ir, IR_MOV_IMM, nreg++, node->type->size, 4);
    return nreg - 1;
  }

  gen_stmt(ir, node);
  return -1;
}

void print_ir(ir_t *ir) {
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
      printf("  store [r%d], r%d\n", ins->lhs, ins->rhs);
      break;
    case IR_LOAD:
      printf("  load r%d, [r%d]\n", ins->lhs, ins->rhs);
      break;
    case IR_CALL:
      printf("  call %s\n", ins->name);
      break;
    case IR_FUNC:
      printf("func %s:\n", ins->name);
      break;
    case IR_LABEL:
      printf("  .L%d:\n", ins->lhs);
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
    case IR_JTRUE:
      printf("  jtrue r%d, .L%d\n", ins->lhs, ins->rhs);
      break;
    case IR_JMP:
      printf("  jmp .L%d\n", ins->lhs);
      break;
    case IR_STORE_VAR:
      printf("  store_var v%d, r%d\n", ins->lhs, ins->rhs);
      break;
    case IR_LOAD_VAR:
      printf("  load_var r%d, v%d\n", ins->lhs, ins->rhs);
      break;
    case IR_LEAVE:
      printf("  leave\n");
      break;
    case IR_LOAD_CONST:
      printf("  load_const r%d, c%d\n", ins->lhs, ins->rhs);
      break;
    case IR_PTR_CAST:
      printf("  ptr_cast r%d\n", ins->lhs);
      break;
    case IR_LOAD_ADDR_VAR:
      printf("  load_addr_var r%d, v%d\n", ins->lhs, ins->rhs);
      break;
    case IR_PUSH:
      printf("  push r%d\n", ins->lhs);
      break;
    case IR_POP:
      printf("  pop r%d\n", ins->lhs);
      break;
    case IR_LOAD_GVAR:
      printf("  load_gvar r%d, %s\n", ins->lhs, ins->name);
      break;
    case IR_LOAD_ADDR_GVAR:
      printf("  load_addr_gvar r%d, %s\n", ins->lhs, ins->name);
      break;
    default:
      error("Unknown operator: %d", ins->op);
    }
  }
}
