#ifndef TILDE_H
#define TILDE_H

typedef int bool;
#define true (1)
#define false (0)

typedef struct
{
    char *str;
    int len;
    int size;
} String;

typedef struct Hashmap Hashmap;

struct List_body;
typedef struct
{
    struct List_body *head;
    struct List_body *tail;
    int len;
} List;

typedef struct
{
    struct List_body *p;
} iter_list;

enum TokenKind
{
    TK_INVALID,
    TK_KEYWORD,
    TK_IDENT,
    TK_CONSTANT,
    TK_STRING,
    TK_PUNCTUATOR,
    TK_ENDFILE,
};

enum PnctID
{
    P_INVALID,

    P_SQR_BRCK_L,
    P_SQR_BRCK_R,
    P_PAREN_L,
    P_PAREN_R,
    P_CRL_BRCK_L,
    P_CRL_BRCK_R,
    P_DOT,
    P_ARRW,
    
    P_PLUS_PLUS,
    P_MINS_MINS,
    P_AMPD,
    P_MULT,
    P_PLUS,
    P_MINS,
    P_TILDE,
    P_EXCM,

    P_DIV,
    P_MOD,
    P_SHIFT_L,
    P_SHIFT_R,
    P_LESS,
    P_GRT,
    P_LESS_EQ,
    P_GRT_EQ,
    P_EQ,
    P_NEQ,
    P_CARET,
    P_VBAR,
    P_AMPD_AMPD,
    P_VBAR_VBAR,
    P_QMARK,
    P_COLON,
    P_SCOLON,
    P_TLEAD,
    P_COMMA,

    P_ASGN,
    P_ASGN_MULT,
    P_ASGN_DIV,
    P_ASGN_MOD,
    P_ASGN_PLUS,
    P_ASGN_MINS,
    P_ASGN_SHIFT_L,
    P_ASGN_SHIFT_R,
    P_ASGN_AMPD,
    P_ASGN_CARET,
    P_ASGN_VBAR,
};

typedef struct
{
    enum TokenKind kind;
    enum PnctID id;
    String *str;
    int row;
    int col;
} Token;

enum TypeType
{
    TT_UNKNOWN,
    TT_BASIC,
    TT_STRUCT_UNION,
    TT_POINTER,
    TT_ARRAY,
    TT_FUNCTION,
};

struct Node;

typedef struct Type
{
    int size;
    int align;
    bool is_const;
    bool is_volatile;
    enum TypeType tt;
    union
    {
        struct /* basic */
        {
            bool is_signed;
        };
        struct /* struct or union */
        {
            bool is_struct;
            List *contents; // List<Node*>
        };
        struct /* function */
        {
            struct Type *ret;
            List *args; // List<Node*>
            bool is_vargs;
            bool is_inline;
        };
        struct /* array */
        {
            bool is_static; /* only function args */
            bool is_varray;   /* variable length array */
            struct Node *asn_exp;
            struct Type *base;
        };
        struct /* pointer */
        {
            struct Type *ptr;
        };
    };
} Type;

enum AST
{
    AST_UNKNOWN,
    AST_IDENT,
    AST_CONSTANT,
    AST_STRING,

    // top-level
    AST_VAR_DECL,
    AST_FUNC_DEF,
    // statement
    AST_LABEL,
    AST_CASE,
    AST_DEFAULT,
    AST_COMP_STMT,
    AST_EXP_STMT,
    AST_IF,
    AST_SWITCH,
    AST_WHILE,
    AST_DO,
    AST_FOR,
    AST_GOTO,
    AST_CONTINUE,
    AST_BREAK,
    AST_RETURN,
};

enum StorageClass
{
    SC_NONE,
    SC_TYPEDEF,
    SC_EXTERN,
    SC_STATIC,
};

typedef struct Node
{
    enum AST kind;
    Type *type;
    union
    {
        struct // var-decl or function definition
        {
            String *name;
            struct Node *init_or_body;
            enum StorageClass sc;
        };
        struct // label
        {
            String *label;
            struct Node *label_stmt;
        };
        struct // case
        {
            struct Node *case_exp;
            struct Node *case_stmt;
        };
        struct // default
        {
            struct Node *default_stmt;
        };
        struct // compound statement
        {
            List *stmts; // List<Node*>
        };
        struct // expression statement
        {
            struct Node *exp_stmt;
        };
        struct // if statement
        {
            struct Node *if_exp;
            struct Node *true_stmt;
            struct Node *false_stmt;
        };
        struct // switch statement
        {
            struct Node *switch_cond;
            struct Node *switch_stmt;
        };
        struct // while statement
        {
            struct Node *while_cond;
            struct Node *while_body;
        };
        struct // do statement
        {
            struct Node *do_cond;
            struct Node *do_body;
        };
        struct // for statement
        {
            struct Node *for_init_exp;
            struct Node *for_cond_exp;
            struct Node *for_step_exp;
            struct Node *for_body;
        };
        struct // goto statement
        {
            String *goto_label;
        };
        struct // return statement
        {
            struct Node *return_exp;
        };
    };
} Node;

// parser.c
void  init_parser(const char *file);
List *parser_top();

// lex.c
bool   open_file(const char *f);
void   close_file();
void   print_token(const Token *tk);
char  *punctuator_to_string(enum PnctID p);
Token *next_token();
void   free_token(Token **tk);

// string.c
String *make_string();
String *new_string(const String *str);
String *new2_string(const char *str);
void   free_string(String **string);
void   append_string(String *dst, const String *src);
void   append2_string(String *dst, char c);
void   append3_string(String *dst, const char *src);
int    cmp_string(const String *s1, const String *s2);
int    cmp2_string(const String *s1, const char *s2);

// list.c
void liberator_void(void *);
List *make_list();
void free_list(List **list, void (*liberator)(void*));
void add_list(List *list, void *data);
bool remove_list(List *list, void (*liberator)(void*), int n);
void *at_list(List *list, int n);
void *pop_list(List *list);
void init_iter_list(const List *list, iter_list *iter);
bool hasnext_iter_list(const iter_list *iter);
void next_iter_list(iter_list *iter);
void *value_iter_list(const iter_list *iter);

// hashmap.c
Hashmap *make_hashmap(int size);
void free_hashmap(Hashmap **h, void (*liberator)(void*));
bool add_hashmap(Hashmap *h, const String *key, void *data);
bool remove_hashmap(Hashmap *h, const String *key, void (*liberator)(void*));
bool exists_hashmap(Hashmap *h, const String *key);
void *search_hashmap(Hashmap *h, const String *key);

// error.c
void print_error(int row, int col, const char *f, ...);
void exit_error(int row, int col, const char *f, ...);
void malloc_error();
void *try_malloc(size_t size);
void *try_calloc(size_t n, size_t size);
void *try_realloc(void *ptr, size_t size);
#endif

