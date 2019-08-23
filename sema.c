#include "sicc.h"

#include <stdlib.h>

static enum {
  STAT_NONE = 0,
  STAT_EXTERNAL,
  STAT_FUNC,
  STAT_WHILE,
  STAT_FOR,
  STAT_SWITCH,
  STAT_EXPR,
} _sema_stat;

// Map of global variable types
static map_t *gvar_types;
// Map of global function types
static map_t *gfuncs;
// Map of Local varialbe types
static map_t *var_types;

// Searching Global/Local Variable types
// If the variable is local, that returns 1
// If the variable is global, that returns 2
static int find_var_types(char *str) {
  if (map_find(var_types, str))
    return 1;
  if (map_find(gvar_types, str))
    return 2;
  return 0;
}

static type_t *get_var_types(char *str) {
  if (find_var_types(str) == 1)
    return map_get(var_types, str);
  if (find_var_types(str) == 2)
    return map_get(gvar_types, str);
  return NULL;
}

// Walking of Semantic Phase
// `stat` is a state that indicates information of where current node is in
void sema_walk(node_t *node, int stat) {
  if (!node)
    return;
  node->flag = calloc(1, sizeof(flag_t));
  switch (node->ty) {
  case ND_EXTERNAL:
    for (int i = 0; i < vec_len(node->decl_list); i++) {
      sema_walk(vec_get(node->decl_list, i), STAT_EXTERNAL);
    }
    for (int i = 0; i < vec_len(node->funcs); i++) {
      sema_walk(vec_get(node->funcs, i), STAT_EXTERNAL);
    }
    break;
  case ND_FUNC:
    map_put(gfuncs, node->str, node->type);
    sema_walk(node->rhs, STAT_FUNC);
    sema_walk(node->lhs, STAT_FUNC);
    free(var_types);
    var_types = new_map();
    break;
  case ND_FUNCS:
    for (int i = 0; i < vec_len(node->funcs); i++) {
      sema_walk(vec_get(node->funcs, i), stat);
    }
    break;
  case ND_ARGS:
    for (int i = 0; i < vec_len(node->args); i++) {
      sema_walk(vec_get(node->args, i), stat);
    }
    break;
  case ND_PARAMS:
    for (int i = 0; i < vec_len(node->params); i++) {
      sema_walk(vec_get(node->params, i), stat);
    }
    break;
  case ND_STMTS: {
    int var_length_before = map_len(var_types);
    for (int i = 0; i < vec_len(node->stmts); i++) {
      sema_walk(vec_get(node->stmts, i), stat);
    }
    int var_len_after = map_len(var_types);
    // for (int i = var_length_before; i < var_len_after; i++)
    //   map_pop(var_types);
  } break;
  case ND_NUM: {
    type_info_t *info = map_get(types, "int");
    node->type = new_type(info->size, info->ty);
  } break;
  case ND_IDENT:
    if (!find_var_types(node->str))
      error("Can't use not defined variable");
    node->type = get_var_types(node->str);
    break;
  case ND_FUNC_CALL:
    sema_walk(node->rhs, STAT_EXPR);
    if (stat == STAT_EXPR)
      node->flag->should_save = true;
    else
      node->flag->should_save = false;
    node->type = map_get(gfuncs, node->str);
    break;
  case ND_EXPR:
    sema_walk(node->lhs, STAT_EXPR);
    sema_walk(node->rhs, STAT_EXPR);
    node->type = node->lhs->type;;
    break;
  case ND_RETURN:
    sema_walk(node->lhs, STAT_EXPR);
    break;
  case ND_IF:
    sema_walk(node->rhs, STAT_EXPR);
    sema_walk(node->lhs, stat);
    break;
  case ND_IF_ELSE:
    sema_walk(node->rhs, STAT_EXPR);
    sema_walk(node->lhs, stat);
    sema_walk(node->else_stmt, stat);
    break;
  case ND_VAR_DEF:
    if (find_var_types(node->str))
      error("Variable redefinition is not allowed: %s", node->str);
    map_put(var_types, node->str, node->type);
    sema_walk(node->lhs, STAT_EXPR);
    if (node->type->ty == TY_ARRAY_NOSIZE) {
      if (node->lhs->ty != ND_INITIALIZER) {
        error("An array without size requires initializer");
      } else {
        node->type->array_size = vec_len(node->lhs->initializer);
        node->type->size = node->type->array_size * node->type->size_deref;
        node->type->ty = TY_ARRAY;
      }
    }
    break;
  case ND_VAR_DECL:
    if (find_var_types(node->str))
      error("Variable redefinition is not allowed: %s", node->str);
    map_put(var_types, node->str, node->type);
    if (node->type->ty == TY_ARRAY_NOSIZE) {
      error("An array without size requires initializer");
    }
    break;
  case ND_EXT_VAR_DEF:
    if (find_var_types(node->str))
      error("Variabe redefinition is not allowed: %s", node->str);
    map_put(gvar_types, node->str, node->type);
    sema_walk(node->lhs, STAT_EXPR);
    if (node->type->ty == TY_ARRAY_NOSIZE) {
      if (node->lhs->ty != ND_INITIALIZER) {
        error("An array without size requires initializer");
      } else {
        node->type->array_size = vec_len(node->lhs->initializer);
        node->type->size = node->type->array_size * node->type->size_deref;
        node->type->ty = TY_ARRAY;
      }
    }
    break;
  case ND_VAR_DECL_LIST:
    for (int i = 0, len = vec_len(node->vars); i < len; i++) {
      sema_walk(vec_get(node->vars, i), stat);
    }
    break;
  case ND_EXT_VAR_DECL:
    if (find_var_types(node->str))
      error("Variable redefinition is not allowed: %s", node->str);
    map_put(gvar_types, node->str, node->type);
    if (node->type->ty == TY_ARRAY_NOSIZE) {
      error("An array without size requires initializer");
    }
    break;
  case ND_DEREF:
    sema_walk(node->lhs, STAT_EXPR);
    node->str = node->lhs->str;
    node->type = node->lhs->type->ptr;
    break;
  case ND_REF:
    sema_walk(node->lhs, STAT_EXPR);
    node->type->ptr = node->lhs->type;
    node->type->size_deref = node->lhs->type->size;
    break;
  case ND_STRING:
    node->type = new_type(8, TY_PTR);
    {
      type_info_t *info = map_get(types, "char");
      node->type->ptr = new_type(info->size, info->ty);
    }
    node->type->size_deref = 1;
    break;
  case ND_CHARACTER:
    node->num = *node->str;
    {
      type_info_t *info = map_get(types, "char");
      node->type = new_type(info->size, info->ty);
    }
    break;
  case ND_SIZEOF:
    if (!node->type) {
      node->ty = ND_NUM;
      sema_walk(node->lhs, stat);
      node->num = node->lhs->type->size;
      node->type = new_type(4, TY_INT);
    } else {
      node->ty = ND_NUM;
      node->num = node->type->size;
      node->type = new_type(4, TY_INT);
    }
    break;
  case ND_WHILE:
    sema_walk(node->rhs, STAT_EXPR);
    sema_walk(node->lhs, STAT_WHILE);
    break;
  case ND_FOR:
    sema_walk(node->init, STAT_EXPR);
    sema_walk(node->cond, STAT_EXPR);
    sema_walk(node->loop, STAT_EXPR);
    sema_walk(node->body, STAT_FOR);

    if (node->init->ty >= ND_VAR_DEF &&
        node->init->ty <= ND_EXT_VAR_DECL) {
      if (node->init->ty == ND_VAR_DECL_LIST) {
        int len = vec_len(node->init->vars);
        for (int i = 0; i < len; i++) {
          map_pop(var_types);
        }
      } else {
        map_pop(var_types);
      }
    }
    break;
  case ND_DEREF_INDEX:
    sema_walk(node->lhs, stat);
    sema_walk(node->rhs, STAT_EXPR);
    int is_left_ptr = 1;
    if (node->lhs->type->size_deref == 0)
      is_left_ptr = 0;
    if (node->lhs->type->size_deref == 0 && node->rhs->type->size_deref == 0)
      error("index is only to use in pointer and array");
    node->type = (is_left_ptr ? node->lhs->type->ptr : node->rhs->type->ptr);
    break;
  case ND_INITIALIZER:
    for (int i = 0; i < node->initializer->len; i++) {
      sema_walk(vec_get(node->initializer, i), STAT_EXPR);
    }
    break;
  case ND_INC_L:
    sema_walk(node->lhs, STAT_EXPR);
    node->type = node->lhs->type;
    if (stat == STAT_EXPR)
      node->flag->should_save = true;
    else
      node->flag->should_save = false;
    break;
  case ND_DEC_L:
    sema_walk(node->lhs, STAT_EXPR);
    node->type = node->lhs->type;
    if (stat == STAT_EXPR)
      node->flag->should_save = true;
    else
      node->flag->should_save = false;
    break;
  case ND_DOT:
    sema_walk(node->lhs, stat);
    if (node->lhs->type->ty != TY_STRUCT)
      error("Dot operator cannot be used for what a type that's not a struct.");
    node->type = map_get(node->lhs->type->member->data, node->str);
    break;
  case ND_ARROW:
    sema_walk(node->lhs, stat);
    if (node->lhs->type->ty != TY_PTR || node->lhs->type->ptr->ty != TY_STRUCT)
      error("Arrow operator cannot be used for what a type that's not a pointer which references to struct.");
    node->type = map_get(node->lhs->type->ptr->member->data, node->str);
    break;
  case ND_SWITCH:
    sema_walk(node->lhs, STAT_EXPR);
    sema_walk(node->rhs, STAT_SWITCH);
    break;
  case ND_CASE:
    if (stat != STAT_SWITCH) {
      error("'case' label can only be used in switch statement.");
    }
    sema_walk(node->lhs, STAT_EXPR);
    break;
  case ND_DEFAULT:
    if (stat != STAT_SWITCH) {
      error("'default' label can only be used in switch statement.");
    }
    break;
  case ND_BREAK:
    if (!(stat == STAT_FOR || stat == STAT_WHILE || stat == STAT_SWITCH))
      error("'break' label can only be used in for, while or switch.");
    break;
  default:
    break;
  }
}

void sema(node_t *node) {
  gvar_types = new_map();
  var_types = new_map();
  gfuncs = new_map();
  sema_walk(node, STAT_NONE);
}
