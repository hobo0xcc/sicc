#include "sicc.h"

#include <stdlib.h>

static map_t *gfuncs;
static map_t *vars;
static int jump_possible = 0;
static int save_retval = 0;

void sema_walk(node_t *node)
{
  node->flags = calloc(1, sizeof(sema_flag_t));
  switch (node->ty) {
    case ND_FUNC:
      map_put(gfuncs, node->str, node->type);
      sema_walk(node->rhs);
      sema_walk(node->lhs);
      free(vars);
      vars = new_map();
      break;
    case ND_FUNCS:
      for (int i = 0; i < vec_len(node->funcs); i++) {
        sema_walk(vec_get(node->funcs, i));
      }
      break;
    case ND_ARGS:
      for (int i = 0; i < vec_len(node->args); i++) {
        sema_walk(vec_get(node->args, i));
      }
      break;
    case ND_PARAMS:
      for (int i = 0; i < vec_len(node->params); i++) {
        sema_walk(vec_get(node->params, i));
      }
      break;
    case ND_STMTS:
      {
        // int size_var = map_len(vars) - 1;
        for (int i = 0; i < vec_len(node->stmts); i++) {
          sema_walk(vec_get(node->stmts, i));
        }
        // int cur_size_var = map_len(vars) - 1;
        // for (int i = size_var; i < cur_size_var; i++)
        //   map_pop(vars);
      }
      break;
    case ND_NUM:
      node->type = map_get(types, "int");
      break;
    case ND_IDENT:
      if (!map_find(vars, node->str))
        error("Can't use not defined variable");
      node->type = map_get(vars, node->str);
      break;
    case ND_FUNC_CALL:
      sema_walk(node->rhs);
      node->type = map_get(gfuncs, node->str);
      break;
    case ND_EXPR:
      sema_walk(node->lhs);
      sema_walk(node->rhs);
      node->type = node->lhs->type;
      break;
    case ND_RETURN:
      sema_walk(node->lhs);
      break;
    case ND_IF:
      sema_walk(node->rhs);
      sema_walk(node->lhs);
      break;
    case ND_IF_ELSE:
      sema_walk(node->rhs);
      sema_walk(node->lhs);
      sema_walk(node->else_stmt);
      break;
    case ND_VAR_DEF:
      map_put(vars, node->str, node->type);
      sema_walk(node->lhs);
      break;
    case ND_VAR_DECL:
      map_put(vars, node->str, node->type);
      break;
    case ND_DEREF:
      sema_walk(node->lhs);
      node->str = node->lhs->str;
      node->type = node->lhs->type->ptr;
      break;
    case ND_STRING:
      node->type = new_type(8, TY_PTR);
      node->type->ptr = map_get(types, "char");
      node->type->ptr_size = 1;
      break;
    case ND_CHARACTER:
      node->num = *node->str;
      node->type = map_get(types, "char");
      break;
    case ND_WHILE:
      jump_possible = 1;
      sema_walk(node->rhs);
      sema_walk(node->lhs);
      break;
    default:
      break;
  }
}

void sema(node_t *node)
{
  vars = new_map();
  gfuncs = new_map();
  sema_walk(node);
}
