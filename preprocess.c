#include "sicc.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define SKIP_SPACE(e)                                                          \
  while (isspace(peek(e, 0)))                                                  \
  eat(e)

map_t *macros;

typedef struct _macro {
  char *name;
  buf_t *macro_buf;
  int macro_size;
  int args_len;
} macro_t;

static char peek(pp_env_t *e, int offset);
static char eat(pp_env_t *e);
static bool is_eof(pp_env_t *e);
static pp_env_t *new_env(char *s);
static macro_t *new_macro(char *name);
static char *get_string(pp_env_t *e);
static char *peek_string(pp_env_t *e, int offset);
static bool cmp_string(pp_env_t *e, char *str, int offset);
static macro_t *parse_macro(pp_env_t *e);
static map_t *parse_args(pp_env_t *e);
static vec_t *parse_params(pp_env_t *e, char *name);
static void replace_macro(pp_env_t *e, buf_t *b);
static void parse_include(pp_env_t *e, buf_t *b);
static void parse_if_section(pp_env_t *e, buf_t *b,
    char *directive);
static void pp_next(pp_env_t *e, buf_t *b);

static char peek(pp_env_t *e, int offset) { return e->s[e->cur_p + offset]; }

static char eat(pp_env_t *e) { return e->s[e->cur_p++]; }

static bool is_eof(pp_env_t *e) { return peek(e, 0) == 0; }

static pp_env_t *new_env(char *s) {
  pp_env_t *e = calloc(1, sizeof(pp_env_t));
  e->s = s;
  e->cur_p = 0;
  return e;
}

static macro_t *new_macro(char *name) {
  macro_t *macro = calloc(1, sizeof(macro_t));
  macro->name = name;
  return macro;
}

static char *get_string(pp_env_t *e) {
  buf_t *b = new_buf();
  if (!isalpha(peek(e, 0)))
    error("Not a string: %c", peek(e, 0));

  while (isalnum(peek(e, 0)) || peek(e, 0) == '_') {
    buf_push(b, eat(e));
  }
  return buf_str(b);
}

static char *peek_string(pp_env_t *e, int offset) {
  buf_t *b = new_buf();
  int i = offset;
  if (!isalpha(peek(e, i)))
    return NULL;
  while (peek(e, i) == ' ')
    i++;
  for (; isalnum(peek(e, i)) || peek(e, i) == '_'; i++) {
    buf_push(b, peek(e, i));
  }
  return buf_str(b);
}

static bool cmp_string(pp_env_t *e, char *str, int offset) {
  char *comp = peek_string(e, offset);
  if (!comp)
    return false;
  if (!strcmp(comp, str))
    return true;
  return false;
}

static macro_t *parse_macro(pp_env_t *e) {
  char *name = get_string(e);
  macro_t *m = new_macro(name);
  SKIP_SPACE(e);
  map_t *args = parse_args(e);
  buf_t *buf = new_buf();
  int args_len = map_len(args);
  char c;
  while ((c = peek(e, 0)) != '\n') {
    if (c == '\\') {
      eat(e);
      SKIP_SPACE(e);
    } else if (isalpha(c) && args_len) {
      // if an argument is used in macro
      char *str = get_string(e);
      if (map_find(args, str)) {
        buf_append(buf, "%s");
      } else {
        buf_append(buf, str);
      }
    } else {
      buf_push(buf, eat(e));
    }
  }
  m->macro_buf = buf;
  m->macro_size = buf_len(buf);
  m->args_len = args_len;
  return m;
}

static map_t *parse_args(pp_env_t *e) {
  map_t *args = new_map();
  if (peek(e, 0) != '(')
    return args;
  eat(e);
  while (peek(e, 0) != ')') {
    map_put(args, get_string(e), NULL);
    char c;
    while ((c = peek(e, 0)) && (isspace(c) || c == ','))
      eat(e);
  }
  eat(e);
  return args;
}

static vec_t *parse_params(pp_env_t *e, char *name) {
  vec_t *params = new_vec();
  SKIP_SPACE(e);
  if (eat(e) != '(')
    error("%s macro requires arguments", name);

  char c;
  buf_t *b = new_buf();
  while ((c = peek(e, 0)) != ')') {
    while ((c = peek(e, 0)) != ',' && c != ')')
      buf_push(b, eat(e));
    vec_push(params, buf_str(b));
    b = new_buf();
    while ((c = peek(e, 0)) && (isspace(c) || c == ','))
      eat(e);
  }
  eat(e);
  return params;
}

static void replace_macro(pp_env_t *e, buf_t *b) {
  char *str = get_string(e);
  if (map_find(macros, str)) {
    macro_t *m = map_get(macros, str);
    int src_len = buf_len(m->macro_buf);
    char *orig_buf = buf_str(m->macro_buf);
    if (m->args_len) {
      vec_t *params = parse_params(e, str);
      char *src = calloc(1, sizeof(char) * src_len);
      for (int i = 0; i < vec_len(params); i++) {
        write_one_fmt(src, orig_buf, (char *)vec_get(params, i));
        strcpy(orig_buf, src);
      }
      buf_append(b, src);
    } else {
      buf_append(b, orig_buf);
    }
  } else {
    buf_append(b, str);
  }
}

static void parse_include(pp_env_t *e, buf_t *b) {
  SKIP_SPACE(e);
  char c;
  if ((c = peek(e, 0)) == '\"') {
    eat(e);
    buf_t *name = new_buf();
    while ((c = peek(e, 0)) && c != '\"')
      buf_push(name, eat(e));
    char *s = read_file(buf_str(name));
    buf_append(b, s);
    eat(e);
  } else if (c == '<') {
    eat(e);
    // ignore
    while (peek(e, 0) != '>' && peek(e, 0))
      eat(e);
    eat(e);
  }
}

static void parse_if_section(pp_env_t *e, buf_t *b,
    char *directive) {
  if (!strcmp(directive, "ifndef")) {
    SKIP_SPACE(e);
    char *name = get_string(e);
    if (!map_find(macros, name)) {
      while (!is_eof(e)) {
        if (peek(e, 0) == '#' && cmp_string(e, "endif", 1)) {
          eat(e);
          get_string(e);
          return;
        }
        pp_next(e, b);
      }
      return;
    } else {
      buf_t *tmp = new_buf();
      while (!is_eof(e)) {
        if (peek(e, 0) == '#' && cmp_string(e, "endif", 1)) {
          eat(e);
          get_string(e);
          return;
        } else if (peek(e, 0) == '#' && cmp_string(e, "else", 1)) {
          pp_next(e, b);
          return;
        }
        pp_next(e, tmp);
      }
      return;
    }
  } else if (!strcmp(directive, "else")) {
    while (!is_eof(e)) {
      if (peek(e, 0) == '#' && cmp_string(e, "endif", 1)) {
        eat(e);
        get_string(e);
        return;
      }
      pp_next(e, b);
    }
  }
}

static void pp_next(pp_env_t *e, buf_t *b) {
  char c = peek(e, 0);
  if (c == '#') {
    eat(e);
    char *ident = get_string(e);
    SKIP_SPACE(e);
    if (!strcmp(ident, "define")) {
      macro_t *m = parse_macro(e);
      map_put(macros, m->name, m);
    } else if (!strcmp(ident, "include")) {
      parse_include(e, b);
    } else if (!strcmp(ident, "ifndef") ||
        !strcmp(ident, "else")) {
      parse_if_section(e, b, ident);
    }
  } else if (isalpha(c)) {
    replace_macro(e, b);
  } else {
    buf_push(b, eat(e));
  }
}

char *preprocess(char *s, pp_env_t *e) {
  if (!e)
    e = new_env(s);
  macros = new_map();
  buf_t *b = new_buf();
  while (!is_eof(e)) {
    pp_next(e, b);
  }

  return buf_str(b);
}
