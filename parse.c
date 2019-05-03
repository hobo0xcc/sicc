#include "sicc.h"

#include <stdlib.h>
#include <string.h>

static int cur = 0;

static token_t *peek(int offset)
{
  return vec_get(tokens, cur + offset);
}

static token_t *eat()
{
  return vec_get(tokens, cur++);
}

static int equal(token_t *tk, char *str)
{
  if (!strcmp(tk->str, str))
    return 1;
  return 0;
}

static int type_equal(token_t *tk, int ty)
{
  if (tk->ty == ty)
    return 1;
  return 0;
}

static void expect(token_t *tk, char *str)
{
  if (!strcmp(tk->str, str))
    return;
  error("%s expected, but got %s", str, tk->str);
}

node_t *new_node(int ty)
{
  node_t *node = calloc(1, sizeof(node_t));
  node->ty = ty;
  return node;
}

static node_t *params();
static node_t *factor();
static node_t *mul_div();
static node_t *add_sub();
static node_t *great_less();
static node_t *assign();

static node_t *stmt();
static node_t *stmts();
static node_t *arguments();
static node_t *function();

static node_t *params()
{
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

static node_t *factor()
{
  if (equal(peek(0), "(")) {
    eat();
    node_t *expr = assign();
    expect(eat(), ")");
    return expr;
  }
  else if (type_equal(peek(0), TK_NUM)) {
    node_t *node = new_node(ND_NUM);
    node->num = atoi(eat()->str);
    return node;
  }
  else if (type_equal(peek(0), TK_IDENT)) {
    if (equal(peek(1), "(")) {
      node_t *node = new_node(ND_FUNC_CALL);
      node->str = eat()->str;
      node->rhs = params();
      return node;
    }
    else {
      node_t *node = new_node(ND_IDENT);
      node->str = eat()->str;
      return node;
    }
  }
  error("Unknown identifier: %s", peek(0)->str);
  return NULL;
}

static node_t *mul_div()
{
  node_t *left = factor();
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
    node_t *right = factor();
    node->lhs = left;
    node->op = op;
    node->rhs = right;
    left = node;
  }
  return left;
}

static node_t *add_sub()
{
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

static node_t *great_less()
{
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

static node_t *assign()
{
  node_t *left = great_less();
  int op;
  for (;;) {
    if (equal(peek(0), "="))
      op = '=';
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

static node_t *stmt()
{
  if (type_equal(peek(0), TK_RETURN)) {
    eat();
    node_t *node = new_node(ND_RETURN);
    node->lhs = assign();
    expect(eat(), ";");
    return node;
  }
  else if (type_equal(peek(0), TK_IF)) {
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
  }
  else {
    node_t *node = assign();
    expect(eat(), ";");
    return node;
  }
}

static node_t *stmts()
{
  node_t *node = new_node(ND_STMTS);
  node->stmts = new_vec();
  expect(eat(), "{");
  while (!equal(peek(0), "}"))
    vec_push(node->stmts, stmt());
  expect(eat(), "}");
  return node;
}

static node_t *arguments()
{
  node_t *node = new_node(ND_ARGS);
  node->args = new_vec();
  expect(eat(), "(");
  while (!equal(peek(0), ")")) {
    node_t *name = new_node(ND_IDENT);
    if (!type_equal(peek(0), TK_IDENT))
      error("Identifier expected, but got %s", peek(0)->str);
    name->str = eat()->str;
    vec_push(node->args, name);
    if (equal(peek(0), ")"))
      break;
    expect(eat(), ",");
  }
  eat();
  return node;
}

static node_t *function()
{
  node_t *node = new_node(ND_FUNC);
  if (!type_equal(peek(0), TK_IDENT))
    error("function name expected, but got %s", peek(0)->str);
  node->str = eat()->str;
  node_t *args = arguments();
  node_t *prog = stmts();
  node->lhs = prog;
  node->rhs = args;
  return node;
}

node_t *parse()
{
  node_t *node = new_node(ND_FUNCS);
  node->funcs = new_vec();
  while (!type_equal(peek(0), TK_EOF)) {
    vec_push(node->funcs, function());
  }
  return node;
}
