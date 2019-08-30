#include "sicc.h"

#include <stdlib.h>

static int nreg = 0;
static int nlabel = 1;
static int nbblabel = 1;
static int nbblabel_start = 1;
static int nbblabel_end = 1;
static int stack_size = 0;
static int cur_stack = 0;

ir_t *new_ir() {
  ir_t *ir = calloc(1, sizeof(ir_t));
  ir->code = new_vec();
  ir->gvars = new_map();
  ir->vars = new_map();
  ir->gfuncs = new_vec();
  ir->const_str = new_vec();
  ir->labels = new_map();
  ir->env = calloc(1, sizeof(ir_env_t));
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
  if (stack_size % 16 != 0)
    stack_size += 16 - (stack_size % 16);
  return cur_stack;
}

static int free_stack(int size) {
  cur_stack -= size;
  return cur_stack;
}

static void cast_reg(int r, type_t *from, type_t *to);

static int gen_lval(ir_t *ir, node_t *node);
static void gen_initializer(ir_t *ir, node_t *node, int offset);
static void gen_stmt(ir_t *ir, node_t *node);
static int gen_expr(ir_t *ir, node_t *node);
static int gen_assign(ir_t *ir, node_t *node, int left, int right);

static int gen_lval(ir_t *ir, node_t *node) {
  if (node->ty == ND_DEREF) {
    int r = gen_ir(ir, node->lhs);
    return r;
  } else if (node->ty == ND_IDENT) {
    if (map_find(ir->vars, node->str)) {
      var_t *var = map_get(ir->vars, node->str);
      emit(ir, IR_LOAD_ADDR_VAR, nreg++, var->offset, -1);
    } else if (map_find(ir->gvars, node->str)) {
      gvar_t *gvar = map_get(ir->gvars, node->str);
      ins_t *ins = emit(ir, IR_LOAD_ADDR_GVAR, nreg++, -1, -1);
      ins->name = gvar->name;
    }
    return nreg - 1;
  } else if (node->ty == ND_DOT) {
    int r = gen_lval(ir, node->lhs);
    int member_offset =
        (int)(intptr_t)map_get(node->lhs->type->member->offset, node->str);
    emit(ir, IR_ADD_IMM, r, member_offset, 8);
    return r;
  } else if (node->ty == ND_ARROW) {
    int r = gen_lval(ir, node->lhs);
    int member_offset =
        (int)(intptr_t)map_get(node->lhs->type->ptr->member->offset, node->str);
    emit(ir, IR_LOAD, r, r, 8);
    emit(ir, IR_ADD_IMM, r, member_offset, 8);
    return r;
  } else if (node->ty == ND_DEREF_INDEX) {
    int left = gen_lval(ir, node->lhs);
    int right = gen_ir(ir, node->rhs);
    int size = node->lhs->type->size_deref;
    // int is_left_ptr = 1;
    // if ((size = node->lhs->type->size_deref) == 0) {
    //   size = node->rhs->type->size_deref;
    //   is_left_ptr = 0;
    // }

    if (size <= 8)
      emit(ir, IR_PTR_CAST, right, -1, size);
    else {
      emit(ir, IR_MOV_IMM, nreg++, size, 8);
      emit(ir, IR_MUL, right, nreg - 1, 8);
      nreg--;
    }
    emit(ir, IR_ADD, left, right, 8);
    nreg--;
    return left;
  } else {
    error("Invalid lvalue: %d", node->ty);
    return -1;
  }
}

static void gen_initializer(ir_t *ir, node_t *node, int offset) {
  int len = vec_len(node->initializer);
  for (int i = 0; i < len; i++) {
    node_t *e = vec_get(node->initializer, i);
    int r = gen_ir(ir, e);
    if (!(e->ty == ND_INITIALIZER))
      emit(ir, IR_STORE_VAR, offset, r, e->type->size);
    offset += e->type->size;
  }
}

static void gen_stmt(ir_t *ir, node_t *node) {
  if (node->ty == ND_NOP)
    return;
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
    if (!node->flag->is_node_static)
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
        if (arg->type->ty == TY_ARRAY)
          emit(ir, IR_LOAD_ARG, offset, i, 8);
        else
          emit(ir, IR_LOAD_ARG, offset, i, arg->type->size);
        arg_stack -= arg->type->size;
      } else {
        int offset = alloc_stack(arg->type->size);
        var_t *var = new_var(offset, arg->type->size);
        map_put(ir->vars, arg->str, var);
        if (arg->type->ty == TY_ARRAY)
          emit(ir, IR_LOAD_ARG, offset, i, 8);
        else
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
    int eval = nbblabel++;
    int prog = nbblabel++;
    int start = nbblabel_start++;
    int end = nbblabel_end++;

    emit(ir, IR_LABEL_BBSTART, start, -1, -1);
    emit(ir, IR_JMP_BB, eval, -1, -1);
    emit(ir, IR_LABEL_BB, prog, -1, -1);
    gen_ir(ir, node->lhs);
    emit(ir, IR_LABEL_BB, eval, -1, -1);
    int r = gen_ir(ir, node->rhs);
    emit(ir, IR_JTRUE_BB, r, prog, -1);
    nreg--;
    emit(ir, IR_LABEL_BBEND, end, -1, -1);
    return;
  }
  if (node->ty == ND_FOR) {
    int cond = nbblabel++;
    int start = nbblabel_start++;
    int end = nbblabel_end++;

    gen_ir(ir, node->init);
    emit(ir, IR_LABEL_BBSTART, start, -1, -1);
    emit(ir, IR_LABEL_BB, cond, -1, -1);
    int r = gen_ir(ir, node->cond);
    if (r != -1) {
      emit(ir, IR_JZERO_BBEND, r, end, -1);
      nreg--;
    }
    ir->env->before_continue = node->loop;
    gen_ir(ir, node->body);
    if (gen_ir(ir, node->loop) != -1) {
      nreg--;
    }
    emit(ir, IR_JMP_BB, cond, -1, -1);
    emit(ir, IR_LABEL_BBEND, end, -1, -1);

    if (node->init->ty >= ND_VAR_DEF && node->init->ty <= ND_EXT_VAR_DECL) {
      if (node->init->ty == ND_VAR_DECL_LIST) {
        int len = vec_len(node->init->vars);
        for (int i = 0; i < len; i++) {
          map_pop(ir->vars);
        }
      } else {
        map_pop(ir->vars);
      }
    }

    return;
  }
  if (node->ty == ND_VAR_DEF) {
    if (map_find(ir->vars, node->str))
      error("%s is already defined", node->str);
    int offset = alloc_stack(node->type->size);
    int r;
    if (node->lhs->ty == ND_INITIALIZER) {
      // if (node->type->ty != TY_ARRAY) {
      //   error("Initializer is only to use to an array");
      // } else {
      gen_initializer(ir, node->lhs, offset);
      var_t *var = new_var(offset, node->type->size);
      map_put(ir->vars, node->str, var);
      // }
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
    if (node->flag->is_node_extern)
      gvar->external = true;
    map_put(ir->gvars, node->str, gvar);
    return;
  }
  if (node->ty == ND_LABEL) {
    int label = nlabel++;
    emit(ir, IR_LABEL, label, -1, -1);
    map_put(ir->labels, node->str, (void *)(intptr_t)label);
    return;
  }
  if (node->ty == ND_GOTO) {
    int label = (int)(intptr_t)map_get(ir->labels, node->str);
    if (!label)
      error("label '%s' not found", node->str);
    emit(ir, IR_JMP, label, -1, -1);
    return;
  }
  if (node->ty == ND_SWITCH) {
    int end = nbblabel_end++;
    int stmt_len = vec_len(node->rhs->stmts);
    vec_t *case_list = new_vec();
    ins_t *jmp_cond = emit(ir, IR_JMP_BB, 0, -1, -1);
    int bb_start = nbblabel;
    for (int i = 0; i < stmt_len; i++) {
      node_t *stmt = vec_get(node->rhs->stmts, i);
      gen_ir(ir, stmt);
      if (stmt->ty == ND_CASE)
        vec_push(case_list, stmt->lhs);
    }
    int case_len = vec_len(case_list);
    emit(ir, IR_JMP_BBEND, end, -1, -1);
    jmp_cond->lhs = nbblabel;
    int i;
    for (i = 0; i < case_len; i++) {
      node_t *case_value = vec_get(case_list, i);
      emit(ir, IR_LABEL_BB, nbblabel++, -1, -1);
      int r = gen_ir(ir, node->lhs);
      int r_value = gen_ir(ir, case_value);
      emit(ir, IR_EQ, r, r_value, 8);
      emit(ir, IR_JTRUE_BB, r, bb_start + i, -1);
      emit(ir, IR_JMP_BB, nbblabel, -1, -1);
      nreg -= 2;
    }
    // Jump to default label.
    emit(ir, IR_LABEL_BB, nbblabel++, -1, -1);
    emit(ir, IR_JMP_BB, bb_start + i, -1, -1);
    // End of switch statement.
    emit(ir, IR_LABEL_BBEND, end, -1, -1);
    return;
  }
  if (node->ty == ND_CASE) {
    emit(ir, IR_LABEL_BB, nbblabel++, -1, -1);
    // int r = nreg++;
    // emit(ir, IR_POP, r, -1, -1);
    // int value = gen_ir(ir, node->lhs);
    // emit(ir, IR_PUSH, r, -1, -1);
    // emit(ir, IR_EQ, value, r, 8);
    // emit(ir, IR_JZERO_BB, value, nbblabel, -1);
    // nreg -= 2;
    return;
  }
  if (node->ty == ND_DEFAULT) {
    emit(ir, IR_LABEL_BB, nbblabel++, -1, -1);
    return;
  }
  if (node->ty == ND_BREAK) {
    emit(ir, IR_JMP_BBEND, nbblabel_end - 1, -1, -1);
    return;
  }
  if (node->ty == ND_CONTINUE) {
    if (ir->env->before_continue) {
      gen_ir(ir, ir->env->before_continue);
      nreg--;
      ir->env->before_continue = NULL;
    }
    emit(ir, IR_JMP_BBSTART, nbblabel_start - 1, -1, -1);
    return;
  }

  error("Not implemented yet: %d", node->ty);
}

static int gen_assign(ir_t *ir, node_t *node, int left, int right) {
  if (left == -1) {
    error("%s is not declared", node->lhs->str);
    return -1;
  } else {
    emit(ir, IR_STORE, left, right, node->lhs->type->size);
    nreg--;
    return nreg;
  }
}

static int gen_expr(ir_t *ir, node_t *node) {
  int left;
  int op;
  int right;
  int size;
  int r;

  op = node->op;
  if (op == '=' || op == OP_PLUS_ASSIGN || op == OP_MINUS_ASSIGN) {
    left = gen_lval(ir, node->lhs);
  } else {
    left = gen_ir(ir, node->lhs);
  }
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
    r = nreg++;
    emit(ir, IR_LOAD, r, left, node->lhs->type->size);
    emit(ir, IR_ADD, r, right, node->lhs->type->size);
    left = gen_assign(ir, node, left, r);
    nreg--;
    break;
  case OP_MINUS_ASSIGN:
    r = nreg++;
    emit(ir, IR_LOAD, r, left, node->lhs->type->size);
    emit(ir, IR_SUB, r, right, node->lhs->type->size);
    left = gen_assign(ir, node, left, r);
    nreg--;
    break;
  case OP_EQUAL:
    emit(ir, IR_EQ, left, right, size);
    break;
  case OP_NOT_EQUAL:
    emit(ir, IR_NEQ, left, right, size);
    break;
  case OP_LOGIC_AND:
    emit(ir, IR_LOGAND, left, right, size);
    break;
  default:
    error("Unknown operator: %d", op);
  }
  nreg--;
  return left;
}

int gen_ir(ir_t *ir, node_t *node) {
  if (!node || node->ty == ND_NOP)
    return -1;
  if (node->ty == ND_EXPR) {
    return gen_expr(ir, node);
  }
  if (node->ty == ND_CAST) {
    int r = gen_ir(ir, node->rhs);
    emit(ir, IR_CAST, r, node->rhs->type->size, node->type->size);
    return r;
  }
  if (node->ty == ND_NUM) {
    emit(ir, IR_MOV_IMM, nreg++, node->num, node->type->size);
    return nreg - 1;
  }
  if (node->ty == ND_IDENT) {
    if (map_find(ir->vars, node->str)) {
      var_t *var = (var_t *)map_get(ir->vars, node->str);
      if (node->type->ty == TY_ARRAY)
        emit(ir, IR_LOAD_ADDR_VAR, nreg++, var->offset, -1);
      else
        emit(ir, IR_LOAD_VAR, nreg++, var->offset, var->size);
      return nreg - 1;
    } else if (map_find(ir->gvars, node->str)) {
      gvar_t *gvar = (gvar_t *)map_get(ir->gvars, node->str);
      ins_t *ins = emit(ir, IR_LOAD_GVAR, nreg++, -1, gvar->size);
      ins->name = gvar->name;
      return nreg - 1;
    } else
      error("Undefined variable: %s", node->str);
  }
  if (node->ty == ND_DEREF) {
    int r = gen_ir(ir, node->lhs);
    emit(ir, IR_LOAD, r, r, node->type->size);
    return r;
  }
  if (node->ty == ND_DEREF_INDEX) {
    int left = gen_lval(ir, node->lhs);
    int right = gen_ir(ir, node->rhs);
    int size = node->lhs->type->size_deref;
    // int is_left_ptr = 1;
    // if ((size = node->lhs->type->size_deref) == 0) {
    //   size = node->rhs->type->size_deref;
    //   is_left_ptr = 0;
    // }

    if (size <= 8)
      emit(ir, IR_PTR_CAST, right, -1, size);
    else {
      emit(ir, IR_MOV_IMM, nreg++, size, 8);
      emit(ir, IR_MUL, right, nreg - 1, 8);
      nreg--;
    }
    emit(ir, IR_ADD, left, right, 8);
    emit(ir, IR_LOAD, left, left, size);
    nreg--;
    return left;
  }
  if (node->ty == ND_REF) {
    int r = gen_lval(ir, node->lhs);
    return r;
  }
  if (node->ty == ND_FUNC_CALL) {
    gen_ir(ir, node->rhs);
    for (int i = 0; i < nreg; i++) {
      emit(ir, IR_PUSH, i, -1, -1);
    }
    emit(ir, IR_MOV_IMM, 7, 0, 1);
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
  if (node->ty == ND_INC_L) {
    int r_value = nreg++;
    int r = gen_lval(ir, node->lhs);
    int tr = nreg++; // tmp reg
    emit(ir, IR_LOAD, r_value, r, node->type->size);
    emit(ir, IR_MOV, tr, r_value, node->type->size);
    emit(ir, IR_ADD_IMM, tr, 1, node->type->size);
    emit(ir, IR_STORE, r, tr, node->type->size);
    nreg -= 2;
    if (!node->flag->should_save)
      nreg--;
    return r_value;
  }
  if (node->ty == ND_DEC_L) {
    int r_value = nreg++;
    int r = gen_lval(ir, node->lhs);
    int tr = nreg++; // tmp reg
    emit(ir, IR_LOAD, r_value, r, node->type->size);
    emit(ir, IR_MOV, tr, r_value, node->type->size);
    emit(ir, IR_SUB_IMM, tr, 1, node->type->size);
    emit(ir, IR_STORE, r, tr, node->type->size);
    nreg -= 2;
    if (!node->flag->should_save)
      nreg--;
    return r_value;
  }
  if (node->ty == ND_DOT) {
    int r = gen_lval(ir, node->lhs);
    int member_offset =
        (int)(intptr_t)map_get(node->lhs->type->member->offset, node->str);
    emit(ir, IR_ADD_IMM, r, member_offset, 8);
    emit(ir, IR_LOAD, r, r, node->type->size);
    return r;
  }
  if (node->ty == ND_ARROW) {
    int r = gen_lval(ir, node->lhs);
    int member_offset =
        (int)(intptr_t)map_get(node->lhs->type->ptr->member->offset, node->str);
    emit(ir, IR_LOAD, r, r, 8);
    emit(ir, IR_ADD_IMM, r, member_offset, 8);
    emit(ir, IR_LOAD, r, r, node->type->size);
    return r;
  }
  if (node->ty == ND_PARAMS) {
    int len = vec_len(node->params);
    for (int i = len - 1; i >= 0; i--) {
      node_t *param = vec_get(node->params, i);
      int r = gen_ir(ir, param);
      if (param->type->ty == TY_ARRAY)
        emit(ir, IR_STORE_ARG, i, r, 8);
      else
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
      printf(".L%d:\n", ins->lhs);
      break;
    case IR_LABEL_BB:
      printf(".LBB%d\n", ins->lhs);
      break;
    case IR_LABEL_BBEND:
      printf(".LBB_END%d\n", ins->lhs);
      break;
    case IR_LABEL_BBSTART:
      printf(".LBB_START%d\n", ins->lhs);
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
    case IR_JTRUE_BB:
      printf("  jtrue r%d, .LBB%d\n", ins->lhs, ins->rhs);
      break;
    case IR_JTRUE_BBEND:
      printf("  jtrue r%d, .LBB_END%d\n", ins->lhs, ins->rhs);
      break;
    case IR_JZERO:
      printf("  jzero r%d, .L%d\n", ins->lhs, ins->rhs);
      break;
    case IR_JZERO_BB:
      printf("  jzero r%d, .LBB%d\n", ins->lhs, ins->rhs);
      break;
    case IR_JZERO_BBEND:
      printf("  jzero r%d, .LBB_END%d\n", ins->lhs, ins->rhs);
      break;
    case IR_JMP:
      printf("  jmp .L%d\n", ins->lhs);
      break;
    case IR_JMP_BB:
      printf("  jmp .LBB%d\n", ins->lhs);
      break;
    case IR_JMP_BBEND:
      printf("  jmp .LBB_END%d\n", ins->lhs);
      break;
    case IR_JMP_BBSTART:
      printf("  jmp .LBB_START%d\n", ins->lhs);
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
    case IR_EQ:
      printf("  eq r%d, r%d\n", ins->lhs, ins->rhs);
      break;
    case IR_NEQ:
      printf("  neq r%d, r%d\n", ins->lhs, ins->rhs);
      break;
    default:
      error("Unknown operator: %d", ins->op);
    }
  }
}
