#include "sicc.h"

#include <stdlib.h>
#include <string.h>

static int cur = 0;

map_t *types;     // type_info_t map
map_t *enum_list; // intptr_t map

// static type_info_t *new_type_info(int size, int ty) {
//   type_info_t *tyinfo = calloc(1, sizeof(type_info_t));
//   tyinfo->size = size;
//   tyinfo->ty = ty;
//   return tyinfo;
// }

static token_t *peek(int offset) { return vec_get(tokens, cur + offset); }

static token_t *eat() { return vec_get(tokens, cur++); }

static int lineno(token_t *tk) { return tk->line; }

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
  error("%s expected, but got %s: line %d", str, tk->str, tk->line);
}

static bool is_typename(token_t *tk) {
  if (map_find(types, peek(0)->str) || type_equal(peek(0), TK_STRUCT) ||
      type_equal(peek(0), TK_TYPEDEF) || type_equal(peek(0), TK_ENUM)) {
    return true;
  }
  return false;
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
  enum_list = new_map();
  map_put(types, "int", new_type(4, TY_INT));
  map_put(types, "char", new_type(1, TY_CHAR));
  map_put(types, "void", new_type(1, TY_VOID));
  map_put(types, "long", new_type(8, TY_LONG));
}

static node_t *params();
static node_t *primary();
static node_t *postfix();
static node_t *unary();
static node_t *cast_expr();
static node_t *mul_expr();
static node_t *add_expr();
static node_t *relation_expr();
static node_t *equal_expr();
static node_t *logic_and_expr();
static node_t *logic_or_expr();
static node_t *cond_expr();
static node_t *assign_expr();
static node_t *const_expr();

static void storage_class(node_t *node);
static type_t *type();
static node_t *init();
static node_t *decl(type_t *ty);
static member_t *struct_declarator();
static type_t *struct_spec();
static type_t *typedef_spec();
static void enum_declarator();
static type_t *enum_spec();
static node_t *ext_decl(type_t *ty);
static node_t *stmt();
static node_t *compound_stmt();
static node_t *arguments();
static node_t *function(type_t *ty);

static node_t *params() {
  node_t *node = new_node(ND_PARAMS);
  node->params = new_vec();
  expect(eat(), "(");
  while (!equal(peek(0), ")")) {
    vec_push(node->params, assign_expr());
    if (equal(peek(0), ")"))
      break;
    expect(eat(), ",");
  }
  eat();
  return node;
}

static node_t *primary() {
  if (equal(peek(0), "(")) {
    eat();
    node_t *expr = assign_expr();
    expect(eat(), ")");
    return expr;
  } else if (type_equal(peek(0), TK_NUM)) {
    node_t *node = new_node(ND_NUM);
    node->num = atoi(eat()->str);
    return node;
  } else if (type_equal(peek(0), TK_IDENT)) {
    if (map_find(enum_list, peek(0)->str)) {
      node_t *node = new_node(ND_NUM);
      node->num = (int)(intptr_t)map_get(enum_list, eat()->str);
      return node;
    }
    node_t *node = new_node(ND_IDENT);
    node->str = eat()->str;
    return node;
  } else if (type_equal(peek(0), TK_STRING)) {
    node_t *node = new_node(ND_STRING);
    node->str = eat()->str;
    return node;
  } else if (type_equal(peek(0), TK_CHARACTER)) {
    node_t *node = new_node(ND_CHARACTER);
    node->str = eat()->str;
    return node;
  }
  error_at(peek(0), "Unknown identifier: %s", peek(0)->str);
  return NULL;
}

static node_t *postfix() {
  node_t *node = primary();
  for (;;) {
    if (equal(peek(0), "[")) {
      node_t *t = new_node(ND_DEREF_INDEX);
      eat();
      node_t *expr = assign_expr();
      t->lhs = node;
      t->rhs = expr;
      expect(eat(), "]");
      node = t;
      continue;
    } else if (equal(peek(0), "(")) {
      node_t *t = new_node(ND_FUNC_CALL);
      t->str = node->str;
      t->rhs = params();
      node = t;
      continue;
    } else if (equal(peek(0), ".")) {
      eat();
      node_t *t = new_node(ND_DOT);
      t->lhs = node;
      token_t *name = eat();
      if (name->ty != TK_IDENT) {
        error_at(name, "Identifier expected but got %s: line %s", name->str, name->line);
      }
      t->str = name->str;
      node = t;
      continue;
    } else if (equal(peek(0), "->")) {
      eat();
      node_t *t = new_node(ND_ARROW);
      t->lhs = node;
      token_t *name = eat();
      if (name->ty != TK_IDENT) {
        error_at(name, "Identifier expected but got %s: line %s", name->str, name->line);
      }
      t->str = name->str;
      node = t;
      continue;
    } else if (equal(peek(0), "++")) {
      eat();
      node_t *t = new_node(ND_INC_L);
      t->lhs = node;
      node = t;
      continue;
    } else if (equal(peek(0), "--")) {
      eat();
      node_t *t = new_node(ND_DEC_L);
      t->lhs = node;
      node = t;
      continue;
    }

    return node;
  }
}

static node_t *unary() {
  if (equal(peek(0), "*")) {
    eat();
    node_t *node = new_node(ND_DEREF);
    node->lhs = unary();
    return node;
  } else if (equal(peek(0), "&")) {
    eat();
    node_t *node = new_node(ND_REF);
    node->lhs = unary();
    node->type = new_type(8, TY_PTR);
    return node;
  } else if (equal(peek(0), "!")) {
    eat();
    node_t *node = new_node(ND_NOT);
    node->lhs = cast_expr();
    return node;
  } else if (equal(peek(0), "-")) {
    eat();
    node_t *node = new_node(ND_MINUS);
    node->lhs = cast_expr();
    return node;
  } else if (equal(peek(0), "sizeof")) {
    eat();
    if (!equal(peek(0), "(")) {
      node_t *item = unary();
      node_t *node = new_node(ND_SIZEOF);
      node->lhs = item;
      return node;
    } else {
      eat();
      node_t *node = new_node(ND_SIZEOF);
      node->type = type();
      expect(eat(), ")");
      return node;
    }
  }
  return postfix();
}

static node_t *cast_expr() {
  if (equal(peek(0), "(")) {
    node_t *node = new_node(ND_CAST);
    eat();
    type_t *ty = type();
    if (!ty) {
      cur--;
      return unary();
    }
    expect(eat(), ")");
    node->type = ty;
    node->rhs = cast_expr();
    return node;
  } else {
    return unary();
  }
}

static node_t *mul_expr() {
  node_t *left = cast_expr();
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
    node_t *right = cast_expr();
    node->lhs = left;
    node->op = op;
    node->rhs = right;
    left = node;
  }
  return left;
}

static node_t *add_expr() {
  node_t *left = mul_expr();
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
    node_t *right = mul_expr();
    node->lhs = left;
    node->op = op;
    node->rhs = right;
    left = node;
  }
  return left;
}

static node_t *relation_expr() {
  node_t *left = add_expr();
  int op;
  for (;;) {
    if (equal(peek(0), ">"))
      op = '>';
    else if (equal(peek(0), "<"))
      op = '<';
    else if (equal(peek(0), ">="))
      op = OP_GREAT_EQ;
    else if (equal(peek(0), "<="))
      op = OP_LESS_EQ;
    else
      break;

    node_t *node = new_node(ND_EXPR);
    eat();
    node_t *right = add_expr();
    node->lhs = left;
    node->op = op;
    node->rhs = right;
    left = node;
  }
  return left;
}

static node_t *equal_expr() {
  node_t *left = relation_expr();
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
    node_t *right = relation_expr();
    node->lhs = left;
    node->op = op;
    node->rhs = right;
    left = node;
  }
  return left;
}

static node_t *logic_and_expr() {
  node_t *left = equal_expr();
  int op;
  for (;;) {
    if (equal(peek(0), "&&"))
      op = OP_LOGIC_AND;
    else
      break;

    node_t *node = new_node(ND_EXPR);
    eat();
    node_t *right = equal_expr();
    node->lhs = left;
    node->op = op;
    node->rhs = right;
    left = node;
  }
  return left;
}

static node_t *logic_or_expr() {
  node_t *left = logic_and_expr();
  int op;
  for (;;) {
    if (equal(peek(0), "||"))
      op = OP_LOGIC_OR;
    else
      break;

    node_t *node = new_node(ND_EXPR);
    eat();
    node_t *right = logic_and_expr();
    node->lhs = left;
    node->op = op;
    node->rhs = right;
    left = node;
  }
  return left;
}

static node_t *cond_expr() {
  node_t *left = logic_or_expr();
  if (equal(peek(0), "?")) {
    eat();
    node_t *lhs = assign_expr();
    expect(eat(), ":");
    node_t *rhs = cond_expr();
    node_t *node = new_node(ND_EXPR);
    node->lhs = left;
    node_t *cond = new_node(ND_COND);
    cond->lhs = lhs;
    cond->rhs = rhs;
    node->rhs = cond;
    node->op = OP_COND;
    return node;
  }
  return left;
}

static node_t *assign_expr() {
  node_t *left = cond_expr();
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
    node_t *right = cond_expr();
    node->lhs = left;
    node->op = op;
    node->rhs = right;
    left = node;
  }
  return left;
}

static node_t *const_expr() { return logic_and_expr(); }

static void storage_class(node_t *node) {
  if (!node->flag)
    node->flag = calloc(1, sizeof(flag_t));

  if (equal(peek(0), "static")) {
    eat();
    node->flag->is_node_static = true;
    return;
  }
  if (equal(peek(0), "extern")) {
    eat();
    node->flag->is_node_extern = true;
    return;
  }
  if (equal(peek(0), "typedef")) {
    typedef_spec();
    return;
  }
  if (equal(peek(0), "const")) {
    eat();
    node->flag->is_node_const = true;
    return;
  }
}

static type_t *type() {
  type_t *type;
  if (equal(peek(0), "struct")) {
    type = struct_spec();
  } else if (equal(peek(0), "enum")) {
    return enum_spec();
  } else {
    token_t *name = peek(0);
    type_t *ty = map_get(types, name->str);
    if (!ty)
      return NULL;
    type = new_type(ty->size, ty->ty);
    type->member = ty->member;
    eat();
  }

  while (equal(peek(0), "*")) {
    eat();
    type_t *ptr_type = new_type(8, TY_PTR);
    ptr_type->ptr = type;
    ptr_type->size_deref = type->size;
    type = ptr_type;
  }
  return type;
}

static node_t *init() {
  if (equal(peek(0), "{")) {
    eat();
    node_t *node = new_node(ND_INITIALIZER);
    node->initializer = new_vec();
    while (!equal(peek(0), "}")) {
      node_t *nexpr = init();
      vec_push(node->initializer, nexpr);
      if (!equal(peek(0), ","))
        break;
      else
        eat();
    }
    expect(eat(), "}");
    return node;
  } else {
    return assign_expr();
  }
}

static void decl_init(node_t *node) {
  token_t *tk = peek(0);
  if (!type_equal(tk, TK_IDENT))
    error_at(peek(0), "Var name expected but got %s", peek(0)->str);
  node->str = eat()->str;
  type_t *array_elem = node->type;
  for (; equal(peek(0), "["); array_elem = node->type) {
    eat();
    if (equal(peek(0), "]")) {
      node->type = new_type(array_elem->size, TY_ARRAY_NOSIZE);
      node->type->size_deref = array_elem->size;
      node->type->ptr = array_elem;
      eat();
    } else {
      node_t *expr = assign_expr();
      if (expr->ty != ND_NUM)
        error_at(peek(0), "Specify an array size with expr is not implemented yet");
      int size = array_elem->size;
      node->type = new_type(size, TY_ARRAY);
      node->type->size_deref = array_elem->size;
      node->type->array_size = expr->num;
      node->type->size = array_elem->size * expr->num;
      node->type->ptr = array_elem;
      expect(eat(), "]");
    }
  }
}

static node_t *decl(type_t *ty) {
  node_t *node = new_node(ND_VAR_DEF);

  if (!ty) {
    storage_class(node);
    if (peek(0)->ty == TK_SEMICOLON) {
      return new_node(ND_NOP);
    }
    node->type = type();
  } else
    node->type = ty;
  if (peek(0)->ty == TK_SEMICOLON) {
    return new_node(ND_NOP);
  }
  if (peek(1)->ty == TK_LPAREN) {
    node_t *func = function(node->type);
    func->flag = node->flag;
    return func;
  }
  decl_init(node);
  if (!equal(peek(0), "=")) {
    node->ty = ND_VAR_DECL;
    return node;
  }
  expect(eat(), "=");
  node->lhs = init();
  return node;
}

static node_t *decl_list() {
  node_t *first = decl(NULL);
  if (!equal(peek(0), ",")) {
    return first;
  }

  node_t *node = new_node(ND_VAR_DECL_LIST);
  node->vars = new_vec();
  vec_push(node->vars, first);
  for (; equal(peek(0), ",");) {
    eat();
    node_t *tmp = new_node(ND_VAR_DECL);
    tmp->type = calloc(1, sizeof(type_t));
    memcpy(tmp->type, first->type, sizeof(type_t));
    decl_init(tmp);
    if (equal(peek(0), "=")) {
      eat();
      tmp->lhs = init();
      tmp->ty = ND_VAR_DEF;
    }
    vec_push(node->vars, tmp);
  }

  return node;
}

static member_t *struct_declarator() {
  expect(eat(), "{");
  member_t *m = calloc(1, sizeof(member_t));
  m->data = new_map();
  m->offset = new_map();
  while (!equal(peek(0), "}")) {
    node_t *node = decl(NULL);
    if (node->ty != ND_VAR_DECL && node->ty != ND_VAR_DECL_LIST) {
      error_at(peek(0), "Variable declaration expected: line %d", peek(0)->line);
    }
    if (node->ty == ND_VAR_DECL_LIST) {
      int len = vec_len(node->vars);
      for (int i = 0; i < len; i++) {
        node_t *var = vec_get(node->vars, i);
        map_put(m->data, var->str, var->type);
        map_put(m->offset, var->str, (void *)(intptr_t)m->size);
        m->size += var->type->size;
      }
    } else {
      map_put(m->data, node->str, node->type);
      map_put(m->offset, node->str, (void *)(intptr_t)m->size);
      m->size += node->type->size;
    }

    expect(eat(), ";");
  }

  expect(eat(), "}");
  return m;
}

static type_t *struct_spec() {
  expect(eat(), "struct");
  token_t *tk = peek(0);
  if (tk->ty == TK_IDENT) {
    if (peek(1)->ty != TK_LBRACE) {
      token_t *name = eat();
      type_t *ty = map_get(types, name->str);
      return ty;
    }
    eat();
    type_t *ty = new_type(0, TY_STRUCT);
    map_put(types, tk->str, ty);
    member_t *m = struct_declarator();
    ty->size = m->size;
    ty->member = m;
    return ty;
  } else if (tk->ty == TK_LBRACE) {
    member_t *m = struct_declarator();
    type_t *ty = new_type(m->size, TY_STRUCT);
    ty->member = m;
    return ty;
  } else {
    error_at(tk, "Variable name expected but got %s", tk->str);
    return NULL;
  }
}

static type_t *typedef_spec() {
  expect(eat(), "typedef");
  node_t *node = decl(NULL);
  if (node->str == NULL)
    return node->type;
  type_t *ty = new_type(node->type->size, node->type->ty);
  ty->member = node->type->member;
  map_put(types, node->str, ty);
  return node->type;
}

static void enum_declarator() {
  expect(eat(), "{");
  int iota = 0;
  while (!equal(peek(0), "}")) {
    token_t *tk = eat();
    if (tk->ty != TK_IDENT)
      error_at(tk, "Identifier expected but got %s: line %d", tk->str, tk->line);
    if (equal(peek(0), "=")) {
      eat();
      token_t *num = eat();
      if (num->ty != TK_NUM)
        error_at(num, "Number expected but got %s: line %d", num->str, num->line);
      iota = atoi(num->str);
    }

    map_put(enum_list, tk->str, (void *)(intptr_t)iota++);
    if (!equal(peek(0), ","))
      break;
    eat();
  }

  expect(eat(), "}");
}

static type_t *enum_spec() {
  expect(eat(), "enum");
  type_t *ty = map_get(types, "int");
  type_t *type = new_type(ty->size, ty->ty);
  if (type_equal(peek(0), TK_IDENT)) {
    map_put(types, eat()->str, ty);
  }
  if (equal(peek(0), "{")) {
    enum_declarator();
  }

  return type;
}

static node_t *ext_decl(type_t *ty) {
  node_t *node = decl(ty);
  if (node->ty == ND_VAR_DECL)
    node->ty = ND_EXT_VAR_DECL;
  else if (node->ty == ND_VAR_DEF)
    node->ty = ND_EXT_VAR_DEF;
  return node;
}

static node_t *stmt() {
  if (type_equal(peek(0), TK_RETURN)) {
    eat();
    node_t *node = new_node(ND_RETURN);
    if (peek(0)->ty == TK_SEMICOLON) {
      expect(eat(), ";");
      node->lhs = NULL;
      return node;
    }
    node->lhs = assign_expr();
    expect(eat(), ";");
    return node;
  } else if (type_equal(peek(0), TK_IF)) {
    eat();
    node_t *node = new_node(ND_IF);
    expect(eat(), "(");
    node->rhs = assign_expr();
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
    node->rhs = assign_expr();
    expect(eat(), ")");
    node->lhs = stmt();
    return node;
  } else if (type_equal(peek(0), TK_FOR)) {
    node_t *node = new_node(ND_FOR);
    eat();
    expect(eat(), "(");
    node_t *init = NULL;
    node_t *cond = NULL;
    node_t *loop = NULL;
    node_t *body;

    token_t *tk = peek(0);
    // for (init; cond; loop) body
    if (map_find(types, tk->str)) {
      init = decl_list();
    } else if (tk->ty == TK_SEMICOLON) {
      init = new_node(ND_NOP);
    } else {
      init = assign_expr();
    }
    expect(eat(), ";");
    if (!equal(peek(0), ";"))
      cond = assign_expr();
    else
      cond = new_node(ND_NOP);
    expect(eat(), ";");
    if (!equal(peek(0), ";"))
      loop = assign_expr();
    else
      loop = new_node(ND_NOP);
    expect(eat(), ")");
    body = stmt();

    node->init = init;
    node->cond = cond;
    node->loop = loop;
    node->body = body;

    return node;
  } else if (type_equal(peek(0), TK_SWITCH)) {
    node_t *node = new_node(ND_SWITCH);
    eat();
    expect(eat(), "(");
    node->lhs = assign_expr();
    expect(eat(), ")");
    node->rhs = stmt();
    return node;
  } else if (type_equal(peek(0), TK_LBRACE)) {
    return compound_stmt();
  } else if (type_equal(peek(0), TK_IDENT) && peek(1)->ty == TK_COLON) {
    node_t *node = new_node(ND_LABEL);
    node->str = eat()->str;
    eat();
    return node;
  } else if (type_equal(peek(0), TK_GOTO)) {
    eat();
    node_t *node = new_node(ND_GOTO);
    if (!type_equal(peek(0), TK_IDENT)) {
      token_t *tk = peek(0);
      error_at(tk, "Identifier expected but got %s: line %d\n", tk->str, tk->line);
    }
    node->str = eat()->str;
    expect(eat(), ";");
    return node;
  } else if (type_equal(peek(0), TK_CASE)) {
    node_t *node = new_node(ND_CASE);
    eat();
    node->lhs = const_expr();
    expect(eat(), ":");
    return node;
  } else if (type_equal(peek(0), TK_DEFAULT)) {
    node_t *node = new_node(ND_DEFAULT);
    eat();
    expect(eat(), ":");
    return node;
  } else if (type_equal(peek(0), TK_BREAK)) {
    node_t *node = new_node(ND_BREAK);
    eat();
    expect(eat(), ";");
    return node;
  } else if (type_equal(peek(0), TK_CONTINUE)) {
    node_t *node = new_node(ND_CONTINUE);
    eat();
    expect(eat(), ";");
    return node;
  } else if (peek(0)->ty == TK_SEMICOLON) {
    eat();
    return new_node(ND_NOP);
  } else {
    node_t *node = assign_expr();
    expect(eat(), ";");
    return node;
  }
}

static node_t *compound_stmt() {
  node_t *node = new_node(ND_STMTS);
  node->stmts = new_vec();
  expect(eat(), "{");
  while (!equal(peek(0), "}")) {
    if (is_typename(peek(0))) {
      node_t *decls = decl_list();
      expect(eat(), ";");
      vec_push(node->stmts, decls);
    } else {
      node_t *st = stmt();
      vec_push(node->stmts, st);
    }
  }
  expect(eat(), "}");
  return node;
}

static node_t *arguments() {
  node_t *node = new_node(ND_ARGS);
  node->args = new_vec();
  expect(eat(), "(");
  while (!equal(peek(0), ")")) {
    if (type_equal(peek(0), TK_VA_SPEC)) {
      eat();
      expect(eat(), ")");
      return node;
    }
    node_t *arg = new_node(ND_VAR_DECL);
    type_t *ty = type();
    if (ty->ty == TY_VOID && equal(peek(0), ")")) {
      eat();
      return node;
    }
    arg->type = ty;
    decl_init(arg);
    vec_push(node->args, arg);
    if (equal(peek(0), ")"))
      break;
    expect(eat(), ",");
  }
  eat();
  return node;
}

static node_t *function(type_t *ty) {
  node_t *node = new_node(ND_FUNC);
  if (!ty) {
    node->type = type();
  } else
    node->type = ty;
  if (!type_equal(peek(0), TK_IDENT))
    error_at(peek(0), "Function name expected, but got %s", peek(0)->str);
  node->str = eat()->str;
  node_t *args = arguments();
  if (equal(peek(0), ";")) {
    eat();
    return new_node(ND_FUNC_DECL);
  }
  node_t *prog = compound_stmt();
  node->lhs = prog;
  node->rhs = args;
  return node;
}

node_t *parse() {
  init_parser();
  node_t *node = new_node(ND_EXTERNAL);
  node->funcs = new_vec();
  node->decl_list = new_vec();

  while (!type_equal(peek(0), TK_EOF)) {
    node_t *d = ext_decl(NULL);
    if (d->ty == ND_FUNC || d->ty == ND_FUNC_DECL)
      vec_push(node->funcs, d);
    else {
      vec_push(node->decl_list, d);
      expect(eat(), ";");
    }
  }

  return node;
}
