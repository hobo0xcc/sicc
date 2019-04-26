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

static node_t *factor();
static node_t *mul_div();
static node_t *add_sub();

static node_t *factor()
{
  if (equal(peek(0), "(")) {
    eat();
    node_t *expr = add_sub();
    expect(eat(), ")");
    return expr;
  }
  else if (type_equal(peek(0), TK_NUM)) {
    node_t *node = new_node(ND_NUM);
    node->num = atoi(eat()->str);
    return node;
  }
  error("Unknown identifier: %s", peek(0)->str);
  return NULL;
}

static node_t *mul_div()
{
  node_t *left = factor();
  while (equal(peek(0), "*") || equal(peek(0), "/")) {
    node_t *op = new_node(*(eat()->str));
    node_t *right = factor();
    node_t *node = new_node(ND_EXPR);
    node->expr = new_vec();
    vec_append(node->expr, 3, left, op, right);
    left = node;
  }
  return left;
}

static node_t *add_sub()
{
  node_t *left = mul_div();
  while (equal(peek(0), "+") || equal(peek(0), "-")) {
    node_t *op = new_node(*(eat()->str));
    node_t *right = mul_div();
    node_t *node = new_node(ND_EXPR);
    node->expr = new_vec();
    vec_append(node->expr, 3, left, op, right);
    left = node;
  }
  return left;
}

node_t *parse()
{
  node_t *node = add_sub();
  return node;
}
