#include "sicc.h"

#include <stdlib.h>
#include <string.h>

static int cur = 0;

map_t *types;

static token_t *peek(int offset) { return vec_get(tokens, cur + offset); }

static token_t *eat() { return vec_get(tokens, cur++); }

static int equal(token_t *tk, char *str) {
    if (!strcmp(tk->str, str))
        return 1;
    return 0;
}

static int type_equal(token_t *tk, int ty) {
    if (tk->ty == ty)
        return 1;
    return 0;
}

static void expect(token_t *tk, char *str) {
    if (!strcmp(tk->str, str))
        return;
    error("%s expected, but got %s", str, tk->str);
}

node_t *new_node(int ty) {
    node_t *node = calloc(1, sizeof(node_t));
    node->ty = ty;
    return node;
}

type_t *new_type(int size, int ty) {
    type_t *type = calloc(1, sizeof(type_t));
    type->size = size;
    type->ty = ty;
    return type;
}

void init_parser() {
    types = new_map();
    map_put(types, "int", new_type(4, TY_INT));
    map_put(types, "char", new_type(1, TY_CHAR));
}

static node_t *params();
static node_t *factor();
static node_t *unary();
static node_t *mul_div();
static node_t *add_sub();
static node_t *great_less();
static node_t *eq_noteq();
static node_t *assign();

static type_t *type();
static node_t *decl();
static node_t *stmt();
static node_t *stmts();
static node_t *arguments();
static node_t *function();

static node_t *params() {
    node_t *node = new_node(ND_PARAMS);
    node->params = new_vec();
    expect(eat(), "(");
    while (!equal(peek(0), ")")) {
        vec_push(node->params, assign());
        if (equal(peek(0), ")"))
            break;
        expect(eat(), ",");
    }
    eat();
    return node;
}

static node_t *factor() {
    if (equal(peek(0), "(")) {
        eat();
        node_t *expr = assign();
        expect(eat(), ")");
        return expr;
    } else if (type_equal(peek(0), TK_NUM)) {
        node_t *node = new_node(ND_NUM);
        node->num = atoi(eat()->str);
        return node;
    } else if (type_equal(peek(0), TK_IDENT)) {
        if (equal(peek(1), "(")) {
            node_t *node = new_node(ND_FUNC_CALL);
            node->str = eat()->str;
            node->rhs = params();
            return node;
        } else {
            node_t *node = new_node(ND_IDENT);
            node->str = eat()->str;
            return node;
        }
    } else if (type_equal(peek(0), TK_STRING)) {
        node_t *node = new_node(ND_STRING);
        node->str = eat()->str;
        return node;
    } else if (type_equal(peek(0), TK_CHARACTER)) {
        node_t *node = new_node(ND_CHARACTER);
        node->str = eat()->str;
        return node;
    }
    error("Unknown identifier: %s", peek(0)->str);
    return NULL;
}

static node_t *unary() {
    if (equal(peek(0), "*")) {
        eat();
        node_t *node = new_node(ND_DEREF);
        node->lhs = unary();
        return node;
    } else if (equal(peek(0), "sizeof")) {
        eat();
        if (!equal(peek(0), "(")) {
            node_t *item = unary();
            node_t *node = new_node(ND_SIZEOF);
            node->type = item->type;
            return node;
        } else {
            eat();
            node_t *node = new_node(ND_SIZEOF);
            node->type = type();
            expect(eat(), ")");
            return node;
        }
    }
    return factor();
}

static node_t *mul_div() {
    node_t *left = unary();
    int op;
    for (;;) {
        if (equal(peek(0), "*"))
            op = '*';
        else if (equal(peek(0), "/"))
            op = '/';
        else
            break;

        node_t *node = new_node(ND_EXPR);
        eat();
        node_t *right = unary();
        node->lhs = left;
        node->op = op;
        node->rhs = right;
        left = node;
    }
    return left;
}

static node_t *add_sub() {
    node_t *left = mul_div();
    int op;
    for (;;) {
        if (equal(peek(0), "+"))
            op = '+';
        else if (equal(peek(0), "-"))
            op = '-';
        else
            break;

        node_t *node = new_node(ND_EXPR);
        eat();
        node_t *right = mul_div();
        node->lhs = left;
        node->op = op;
        node->rhs = right;
        left = node;
    }
    return left;
}

static node_t *great_less() {
    node_t *left = add_sub();
    int op;
    for (;;) {
        if (equal(peek(0), ">"))
            op = '>';
        else if (equal(peek(0), "<"))
            op = '<';
        else
            break;

        node_t *node = new_node(ND_EXPR);
        eat();
        node_t *right = add_sub();
        node->lhs = left;
        node->op = op;
        node->rhs = right;
        left = node;
    }
    return left;
}

static node_t *eq_noteq() {
    node_t *left = great_less();
    int op;
    for (;;) {
        if (equal(peek(0), "=="))
            op = OP_EQUAL;
        else if (equal(peek(0), "!="))
            op = OP_NOT_EQUAL;
        else 
            break;

        node_t *node = new_node(ND_EXPR);
        eat();
        node_t *right = great_less();
        node->lhs = left;
        node->op = op;
        node->rhs = right;
        left = node;
    }
    return left;
}

static node_t *assign() {
    node_t *left = eq_noteq();
    int op;
    for (;;) {
        if (equal(peek(0), "="))
            op = '=';
        else if (equal(peek(0), "+="))
            op = OP_PLUS_ASSIGN;
        else if (equal(peek(0), "-="))
            op = OP_MINUS_ASSIGN;
        else
            break;

        node_t *node = new_node(ND_EXPR);
        eat();
        node_t *right = eq_noteq();
        node->lhs = left;
        node->op = op;
        node->rhs = right;
        left = node;
    }
    return left;
}

static type_t *type() {
    char *type_name = peek(0)->str;
    type_t *type = (type_t *)map_get(types, type_name);
    eat();

    while (equal(peek(0), "*")) {
        eat();
        type_t *ptr_type = new_type(8, TY_PTR);
        ptr_type->ptr = type;
        ptr_type->size_deref = type->size;
        type = ptr_type;
    }
    return type;
}

static node_t *decl() {
    node_t *node = new_node(ND_VAR_DEF);
    node->type = type();
    if (!type_equal(peek(0), TK_IDENT))
        error("Var name expected but got %s", peek(0)->str);
    node->str = eat()->str;
    if (!equal(peek(0), "=")) {
        node->ty = ND_VAR_DECL;
        return node;
    }
    expect(eat(), "=");
    node->lhs = assign();
    return node;
}

static node_t *stmt() {
    if (type_equal(peek(0), TK_RETURN)) {
        eat();
        node_t *node = new_node(ND_RETURN);
        node->lhs = assign();
        expect(eat(), ";");
        return node;
    } else if (type_equal(peek(0), TK_IF)) {
        eat();
        node_t *node = new_node(ND_IF);
        expect(eat(), "(");
        node->rhs = assign();
        expect(eat(), ")");
        node->lhs = stmt();
        if (type_equal(peek(0), TK_ELSE)) {
            eat();
            node->else_stmt = stmt();
            node->ty = ND_IF_ELSE;
        }
        return node;
    } else if (type_equal(peek(0), TK_WHILE)) {
        node_t *node = new_node(ND_WHILE);
        eat();
        expect(eat(), "(");
        node->rhs = assign();
        expect(eat(), ")");
        node->lhs = stmt();
        return node;
    } else if (type_equal(peek(0), TK_LBRACE)) {
        return stmts();
    } else if (map_find(types, peek(0)->str)) {
        node_t *node = decl();
        expect(eat(), ";");
        return node;
    } else {
        node_t *node = assign();
        expect(eat(), ";");
        return node;
    }
}

static node_t *stmts() {
    node_t *node = new_node(ND_STMTS);
    node->stmts = new_vec();
    expect(eat(), "{");
    while (!equal(peek(0), "}"))
        vec_push(node->stmts, stmt());
    expect(eat(), "}");
    return node;
}

static node_t *arguments() {
    node_t *node = new_node(ND_ARGS);
    node->args = new_vec();
    expect(eat(), "(");
    while (!equal(peek(0), ")")) {
        type_t *ty = type();
        node_t *name = new_node(ND_VAR_DECL);
        if (!type_equal(peek(0), TK_IDENT))
            error("Identifier expected, but got %s", peek(0)->str);
        name->str = eat()->str;
        name->type = ty;
        vec_push(node->args, name);
        if (equal(peek(0), ")"))
            break;
        expect(eat(), ",");
    }
    eat();
    return node;
}

static node_t *function() {
    node_t *node = new_node(ND_FUNC);
    node->type = type();
    if (!type_equal(peek(0), TK_IDENT))
        error("function name expected, but got %s", peek(0)->str);
    node->str = eat()->str;
    node_t *args = arguments();
    node_t *prog = stmt();
    node->lhs = prog;
    node->rhs = args;
    return node;
}

node_t *parse() {
    init_parser();
    node_t *node = new_node(ND_FUNCS);
    node->funcs = new_vec();
    while (!type_equal(peek(0), TK_EOF)) {
        vec_push(node->funcs, function());
    }
    return node;
}
