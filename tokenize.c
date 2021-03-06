#include "sicc.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

vec_t *tokens = NULL;
int pos = 0;

static struct keyword {
  char *str;
  int ty;
} keywords[] = {
    {"return", TK_RETURN},     {"if", TK_IF},
    {"else", TK_ELSE},         {"int", TK_INT},
    {"char", TK_CHAR},         {"while", TK_WHILE},
    {"sizeof", TK_SIZEOF},     {"for", TK_FOR},
    {"struct", TK_STRUCT},     {"typedef", TK_TYPEDEF},
    {"goto", TK_GOTO},         {"switch", TK_SWITCH},
    {"case", TK_CASE},         {"default", TK_DEFAULT},
    {"break", TK_BREAK},       {"enum", TK_ENUM},
    {"continue", TK_CONTINUE}, {NULL, 0},
};

static int check_ident_type(char *str) {
  for (int i = 0; keywords[i].ty != 0; i++) {
    if (!strcmp(str, keywords[i].str))
      return keywords[i].ty;
  }
  return TK_IDENT;
}

static char get_escape_char(char c, char **s) {
  if (c == '\\') {
    if (**s == '0') {
      *s += 1;
      return '\0';
    } else if (**s == 'a') {
      *s += 1;
      return '\a';
    } else if (**s == 'b') {
      *s += 1;
      return '\b';
    } else if (**s == 'f') {
      *s += 1;
      return '\f';
    } else if (**s == 'n') {
      *s += 1;
      return '\n';
    } else if (**s == 'r') {
      *s += 1;
      return '\r';
    } else if (**s == '\t') {
      *s += 1;
      return '\t';
    } else if (**s == '\\') {
      *s += 1;
      return '\\';
    } else if (**s == '\'') {
      *s += 1;
      return '\'';
    } else if (**s == '\"') {
      *s += 1;
      return '\"';
    } else
      return '\\';
  }
  return c;
}

static char next(char **s) {
  pos++;
  return *((*s)++);
}

token_t *make_token(int ty, char *str, int line) {
  token_t *tk = malloc(sizeof(token_t));
  tk->ty = ty;
  tk->str = str;
  tk->line = line;
  tk->pos = pos;
  return tk;
}

void tokenize(char *s) {
  char c;
  int n = 0, line = 1;
  tokens = new_vec();

  while ((c = *s)) {
    next(&s);

    if (c == '\n') {
      line++;
      pos = 0;
      continue;
    }
    if (isspace(c)) {
      continue;
    }
    if (c == '+') {
      if (*s == '=') {
        vec_push(tokens, make_token(TK_PLUS_ASSIGN, "+=", line));
        next(&s);
      } else if (*s == '+') {
        vec_push(tokens, make_token(TK_PLUS_PLUS, "++", line));
        next(&s);
      } else {
        vec_push(tokens, make_token(TK_PLUS, "+", line));
      }
      continue;
    }
    if (c == '-') {
      if (*s == '=') {
        vec_push(tokens, make_token(TK_MINUS_ASSIGN, "-=", line));
        next(&s);
      } else if (*s == '-') {
        vec_push(tokens, make_token(TK_MINUS_MINUS, "--", line));
        next(&s);
      } else if (*s == '>') {
        vec_push(tokens, make_token(TK_ARROW, "->", line));
        next(&s);
      } else {
        vec_push(tokens, make_token(TK_MINUS, "-", line));
      }
      continue;
    }
    if (c == '*') {
      vec_push(tokens, make_token(TK_ASTERISK, "*", line));
      continue;
    }
    if (c == '/') {
      if (*s == '/') {
        while (*s != '\n')
          next(&s);
        continue;
      } else if (*s == '*') {
        while (*s) {
          s++;
          if (*s == '*' && *(s + 1) == '/')
            break;
        }
        if (*s == '\0')
          error("Multiple line comments must be ending as '*/'");
        next(&s);
        next(&s); // skip '*' and '/'
        continue;
      }
      vec_push(tokens, make_token(TK_SLASH, "/", line));
      continue;
    }
    if (c == '=') {
      if (*s == '=') {
        next(&s);
        vec_push(tokens, make_token(TK_EQUAL, "==", line));
        continue;
      } else {
        vec_push(tokens, make_token(TK_ASSIGN, "=", line));
        continue;
      }
    }
    if (c == '>') {
      if (*s == '=') {
        s++;
        vec_push(tokens, make_token(TK_GREAT_EQ, ">=", line));
      } else {
        vec_push(tokens, make_token(TK_GREAT, ">", line));
      }
      continue;
    }
    if (c == '<') {
      if (*s == '=') {
        s++;
        vec_push(tokens, make_token(TK_LESS_EQ, "<=", line));
      } else {
        vec_push(tokens, make_token(TK_LESS, "<", line));
      }
      continue;
    }
    if (c == '(') {
      vec_push(tokens, make_token(TK_LPAREN, "(", line));
      continue;
    }
    if (c == ')') {
      vec_push(tokens, make_token(TK_RPAREN, ")", line));
      continue;
    }
    if (c == '{') {
      vec_push(tokens, make_token(TK_LBRACE, "{", line));
      continue;
    }
    if (c == '}') {
      vec_push(tokens, make_token(TK_RBRACE, "}", line));
      continue;
    }
    if (c == ';') {
      vec_push(tokens, make_token(TK_SEMICOLON, ";", line));
      continue;
    }
    if (c == ':') {
      vec_push(tokens, make_token(TK_COLON, ":", line));
      continue;
    }
    if (c == ',') {
      vec_push(tokens, make_token(TK_COMMA, ",", line));
      continue;
    }
    if (c == '.') {
      if (*s == '.' && *(s+1) == '.') {
        s += 2;
        vec_push(tokens, make_token(TK_VA_SPEC, "...", line));
      } else {
        vec_push(tokens, make_token(TK_DOT, ".", line));
      }
      continue;
    }
    if (c == '\"') {
      buf_t *b = new_buf();
      while (*s != '\"')
        buf_push(b, next(&s));
      next(&s);
      vec_push(tokens, make_token(TK_STRING, buf_str(b), line));
      continue;
    }
    if (c == '\'') {
      char ch = next(&s);
      buf_t *b = new_buf();
      buf_push(b, get_escape_char(ch, &s));
      next(&s);
      vec_push(tokens, make_token(TK_CHARACTER, buf_str(b), line));
      continue;
    }
    if (c == '!') {
      if (*s == '=') {
        next(&s);
        vec_push(tokens, make_token(TK_NOT_EQUAL, "!=", line));
        continue;
      } else {
        vec_push(tokens, make_token(TK_NOT, "!", line));
        continue;
      }
    }
    if (c == '?') {
      vec_push(tokens, make_token(TK_QUESTION, "?", line));
      continue;
    }
    if (c == '&') {
      if (*s == '&') {
        next(&s);
        vec_push(tokens, make_token(TK_AND_AND, "&&", line));
      } else {
        vec_push(tokens, make_token(TK_AND, "&", line));
      }
      continue;
    }
    if (c == '|') {
      if (*s == '|') {
        next(&s);
        vec_push(tokens, make_token(TK_OR_OR, "||", line));
      } else {
        vec_push(tokens, make_token(TK_OR, "|", line));
      }
      continue;
    }
    if (c == '[') {
      vec_push(tokens, make_token(TK_LBRACKET, "[", line));
      continue;
    }
    if (c == ']') {
      vec_push(tokens, make_token(TK_RBRACKET, "]", line));
      continue;
    }

    if (isdigit(c)) {
      buf_t *b = new_buf();
      buf_push(b, c);
      while (isdigit(*s))
        buf_push(b, next(&s));

      vec_push(tokens, make_token(TK_NUM, buf_str(b), line));
      continue;
    }
    if (isalpha(c) || c == '_') {
      buf_t *b = new_buf();
      buf_push(b, c);
      while (isalnum(*s) || *s == '_')
        buf_push(b, next(&s));
      char *str = buf_str(b);
      vec_push(tokens, make_token(check_ident_type(str), str, line));
      continue;
    }

    error("Unknown character: %c", c);
  }
  vec_push(tokens, make_token(TK_EOF, "\0", line));

  return;
}
