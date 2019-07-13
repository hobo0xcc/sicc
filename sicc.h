#ifndef SICC_H
#define SICC_H

#include <stdbool.h>
#include <stdio.h>

enum _token_enum {
    TK_EOF = 256,
    TK_NUM,
    TK_IDENT,
    TK_PLUS,
    TK_MINUS,
    TK_ASTERISK,
    TK_SLASH,
    TK_ASSIGN,
    TK_PLUS_ASSIGN,
    TK_MINUS_ASSIGN,
    TK_PLUS_PLUS,
    TK_MINUS_MINUS,
    TK_GREATER,
    TK_LESS,
    TK_NOT_EQUAL,
    TK_EQUAL,
    TK_NOT,

    TK_LPAREN,
    TK_RPAREN,
    TK_LBRACE,
    TK_RBRACE,
    TK_LBRACKET,
    TK_RBRACKET,
    TK_SEMICOLON,
    TK_COMMA,

    TK_RETURN,
    TK_IF,
    TK_ELSE,
    TK_WHILE,

    TK_SIZEOF,

    TK_STRING,
    TK_CHARACTER,

    TK_INT,
    TK_CHAR,
};

enum _type_enum {
    TY_INT,
    TY_CHAR,
    TY_PTR,
    TY_STRUCT,
    TY_ARRAY,
    TY_ARRAY_NOSIZE,
};

enum _op_enum {
    OP_PLUS_ASSIGN,
    OP_MINUS_ASSIGN,
    OP_EQUAL,
    OP_NOT_EQUAL,
};

enum _node_enum {
    ND_FUNC = 256,
    ND_FUNCS = 257,
    ND_ARGS = 258,
    ND_PARAMS = 259,
    ND_STMTS = 260,
    ND_NUM = 261,
    ND_IDENT = 262,
    ND_FUNC_CALL = 263,
    ND_EXPR = 264,
    ND_RETURN = 265,
    ND_IF = 266,
    ND_IF_ELSE = 267,
    ND_VAR_DEF = 268,
    ND_VAR_DECL = 269,
    ND_DEREF = 270,
    ND_STRING = 271,
    ND_CHARACTER = 272,
    ND_WHILE = 273,
    ND_SIZEOF = 274,
    ND_DEREF_INDEX = 275,
    ND_DEREF_LVAL = 276,
    ND_DEREF_INDEX_LVAL = 277,
    ND_IDENT_LVAL = 278,
    ND_INITIALIZER = 279,
};

enum _ir_enum {
    IR_MOV_IMM,    // Move immediate value to register
    IR_MOV_RETVAL, // Move return value to register
    IR_STORE_ARG,  // Store
    IR_LOAD_ARG,   // Load
    IR_ADD,        // Add
    IR_SUB,        // Subtract
    IR_MUL,        // Multiply
    IR_DIV,        // Divided
    IR_GREAT,      // Greater
    IR_LESS,       // Less
    IR_STORE,      // Store register to var
    IR_LOAD,       // Load var to register
    IR_CALL,       // Call function
    IR_FUNC,       // Function
    IR_LABEL,      // Label
    IR_ALLOC,      // Alloc vars
    IR_FREE,       // Free vars
    IR_RET,        // Return register
    IR_RET_NONE,   // Return none
    // IR_SAVE_REG,      Save register
    // IR_REST_REG,      Restore register
    IR_JMP,       // Jmp
    IR_JTRUE,     // Jmp if true(1)
    IR_STORE_VAR, // Store reg to var
    IR_LOAD_VAR,  // Load var to reg
    IR_LOAD_ADDR,
    IR_LEAVE,
    IR_LOAD_CONST,
    IR_PTR_CAST,
    IR_EQ,
    IR_NEQ,
    IR_LOAD_ADDR_VAR,
};

typedef struct _vec {
    int cap, len;
    void **data;
} vec_t;

typedef struct _map {
    int len;
    vec_t *keys;
    vec_t *items;
} map_t;

typedef struct _buf {
    int len, cap;
    char *data;
} buf_t;

typedef struct _token {
    int ty;
    char *str;
    int line;
} token_t;

typedef struct _type_info {
    int size;
    int ty;
} type_info_t;

typedef struct _type {
    int size;
    struct _type *ptr;
    int ty;
    char *name;
    int size_deref;
    int array_size;
} type_t;

typedef struct _node {
    int ty;
    struct _node *lhs;
    struct _node *rhs;
    int op;
    char *str;
    int num;
    int size;
    type_t *type;
    struct _node *else_stmt;
    vec_t *stmts;
    vec_t *funcs;
    vec_t *args;
    vec_t *params;
    vec_t *initializer;
} node_t;

typedef struct _ins {
    int op;
    int lhs;
    int rhs;

    int size;
    char *name;
} ins_t;

typedef struct _var {
    int offset;
    int size;
} var_t;

typedef struct _ir {
    vec_t *code;      // ins_t list
    map_t *vars;      // var_t map
    vec_t *gfuncs;    // char * list
    vec_t *const_str; // char * list
    int len;          // code length
    int stack_size;   // max stack size in function
} ir_t;

extern vec_t *tokens;
extern map_t *types;

/* util.c */
char *read_file(char *name);
void write_one_fmt(char *dst, char *orig, char *str);

vec_t *new_vec();
void grow_vec(vec_t *v, int len);
void vec_push(vec_t *v, void *p);
void vec_pop(vec_t *v);
void vec_append(vec_t *v, int len, ...);
void vec_set(vec_t *v, int pos, void *p);
void *vec_get(vec_t *v, int pos);
size_t vec_len(vec_t *v);

map_t *new_map();
void map_put(map_t *m, char *key, void *item);
void map_set(map_t *m, char *key, void *item);
void *map_get(map_t *m, char *key);
int map_find(map_t *m, char *key);
int map_index(map_t *m, char *key);
void map_pop(map_t *m);
size_t map_len(map_t *m);

buf_t *new_buf();
void grow_buf(buf_t *b, int len);
void buf_push(buf_t *b, char c);
void buf_append(buf_t *b, char *str);
void buf_appendn(buf_t *b, char *str, int n);
size_t buf_len(buf_t *b);
char *buf_str(buf_t *b);

/* debug.c */
void debug_tokens(vec_t *tokens);
void debug_node(node_t *node);
void debug_ir(char *filename);
void debug(char *s);

/* preprocess.c */
extern map_t *macros;

char *preprocess(char *s);

/* tokenize.c */
void tokenize(char *s);

/* parse.c */
node_t *new_node(int ty);
type_t *new_type(int size, int ty);
void init_parser();
node_t *parse();

/* sema.c */
void sema_walk(node_t *node);
void sema(node_t *node);

/* irgen.c */
ir_t *new_ir();
int gen_ir(ir_t *ir, node_t *node);
void print_ir(ir_t *ir);

/* asmgen.c */
void gen_asm(ir_t *ir);

/* error.c */
void error(const char *fmt, ...);

#endif
