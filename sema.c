#include "sicc.h"

#include <stdlib.h>

static map_t *gfuncs;
static map_t *vars;

void sema_walk(node_t *node)
{
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
      for (int i = 0; i < vec_len(node->stmts); i++) {
        sema_walk(vec_get(node->stmts, i));
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
