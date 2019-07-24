#include "sicc.h"

#include <stdlib.h>

static map_t *gvar_types;
static map_t *gfuncs;
static map_t *var_types;
static int save_retval = 0;

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

void sema_walk(node_t *node) {
    //     node->flags = calloc(1, sizeof(sema_flag_t));
    switch (node->ty) {
    case ND_EXTERNAL:
        for (int i = 0; i < vec_len(node->decl_list); i++) {
            sema_walk(vec_get(node->decl_list, i));
        }
        for (int i = 0; i < vec_len(node->funcs); i++) {
            sema_walk(vec_get(node->funcs, i));
        }
        break;
    case ND_FUNC:
        map_put(gfuncs, node->str, node->type);
        sema_walk(node->rhs);
        sema_walk(node->lhs);
        free(var_types);
        var_types = new_map();
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
    case ND_STMTS: {
        // int size_var = map_len(vars) - 1;
        for (int i = 0; i < vec_len(node->stmts); i++) {
            sema_walk(vec_get(node->stmts, i));
        }
        // int cur_size_var = map_len(vars) - 1;
        // for (int i = size_var; i < cur_size_var; i++)
        //   map_pop(vars);
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
        sema_walk(node->rhs);
        node->type = map_get(gfuncs, node->str);
        break;
    case ND_EXPR:
        sema_walk(node->lhs);
        sema_walk(node->rhs);
        node->type = node->lhs->type;
        if (node->lhs->ty == ND_IDENT && node->op == '=')
            node->lhs->ty = ND_IDENT_LVAL;
        else if (node->lhs->ty == ND_DEREF && node->op == '=')
            node->lhs->ty = ND_DEREF_LVAL;
        else if (node->lhs->ty == ND_DEREF_INDEX && node->op == '=')
            node->lhs->ty = ND_DEREF_INDEX_LVAL;
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
        map_put(var_types, node->str, node->type);
        sema_walk(node->lhs);
        if (node->type->ty == TY_ARRAY_NOSIZE) {
            if (node->lhs->ty != ND_INITIALIZER) {
                error("An array without size requires initializer");
            } else {
                node->type->array_size = vec_len(node->lhs->initializer);
                node->type->size *= node->type->array_size;
                node->type->ty = TY_ARRAY;
            }
        }
        break;
    case ND_VAR_DECL:
        map_put(var_types, node->str, node->type);
        if (node->type->ty == TY_ARRAY_NOSIZE) {
            error("An array without size requires initializer");
        }
        break;
    case ND_EXT_VAR_DEF:
        map_put(gvar_types, node->str, node->type);
        sema_walk(node->lhs);
        if (node->type->ty == TY_ARRAY_NOSIZE) {
            if (node->lhs->ty != ND_INITIALIZER) {
                error("An array without size requires initializer");
            } else {
                node->type->array_size = vec_len(node->lhs->initializer);
                node->type->size *= node->type->array_size;
                node->type->ty = TY_ARRAY;
            }
        }
        break;
    case ND_EXT_VAR_DECL:
        map_put(gvar_types, node->str, node->type);
        if (node->type->ty == TY_ARRAY_NOSIZE) {
            error("An array without size requires initializer");
        }
        break;
    case ND_DEREF:
    case ND_DEREF_LVAL:
        sema_walk(node->lhs);
        node->str = node->lhs->str;
        node->type = node->lhs->type->ptr;
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
    case ND_WHILE:
        sema_walk(node->rhs);
        sema_walk(node->lhs);
        break;
    case ND_DEREF_INDEX:
    case ND_DEREF_INDEX_LVAL:
        sema_walk(node->lhs);
        sema_walk(node->rhs);
        if (node->lhs->ty == ND_IDENT)
            node->lhs->ty = ND_IDENT_LVAL;
        int is_left_ptr = 1;
        if (node->lhs->type->size_deref == 0)
            is_left_ptr = 0;
        if (node->lhs->type->size_deref == 0 &&
            node->rhs->type->size_deref == 0)
            error("index is only to use in pointer and array");
        node->type =
            (is_left_ptr ? node->lhs->type->ptr : node->rhs->type->ptr);
        break;
    case ND_INITIALIZER:
        for (int i = 0; i < node->initializer->len; i++) {
            sema_walk(vec_get(node->initializer, i));
        }
        break;
    default:
        break;
    }
}

void sema(node_t *node) {
    gvar_types = new_map();
    var_types = new_map();
    gfuncs = new_map();
    sema_walk(node);
}
