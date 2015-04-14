#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tilde.h"

Type   *void_t = &(Type){.size = 0, .align = 0, .is_const = false, .is_volatile = false, .tt = TT_BASIC};
Type   *bool_t = &(Type){.size = 1, .align = 1, .is_const = false, .is_volatile = false, .tt = TT_BASIC};
Type   *char_t = &(Type){.size = 1, .align = 1, .is_const = false, .is_volatile = false, .tt = TT_BASIC, .is_signed = true};
Type  *uchar_t = &(Type){.size = 1, .align = 1, .is_const = false, .is_volatile = false, .tt = TT_BASIC, .is_signed = false};
Type  *short_t = &(Type){.size = 2, .align = 2, .is_const = false, .is_volatile = false, .tt = TT_BASIC, .is_signed = true};
Type *ushort_t = &(Type){.size = 2, .align = 2, .is_const = false, .is_volatile = false, .tt = TT_BASIC, .is_signed = false};
Type    *int_t = &(Type){.size = 4, .align = 4, .is_const = false, .is_volatile = false, .tt = TT_BASIC, .is_signed = true};
Type   *uint_t = &(Type){.size = 4, .align = 4, .is_const = false, .is_volatile = false, .tt = TT_BASIC, .is_signed = false};
Type   *long_t = &(Type){.size = 8, .align = 8, .is_const = false, .is_volatile = false, .tt = TT_BASIC, .is_signed = true};
Type  *ulong_t = &(Type){.size = 8, .align = 8, .is_const = false, .is_volatile = false, .tt = TT_BASIC, .is_signed = false};
Type  *float_t = &(Type){.size = 4, .align = 4, .is_const = false, .is_volatile = false, .tt = TT_BASIC};
Type *double_t = &(Type){.size = 8, .align = 8, .is_const = false, .is_volatile = false, .tt = TT_BASIC};
Type    *ptr_t = &(Type){.size = 8, .align = 8, .is_const = false, .is_volatile = false, .tt = TT_POINTER};

List *stack; // List<Token*>

/* prototypes */
static Node *make_extdecl();
static void make_decl_spec(Node *node);
static bool make_storage_class_spec(Node *node);
static Type *make_type_spec();
static bool make_type_qual(bool *isc, bool *isv);
static bool make_func_spec();
static void make_declarator(Node *node);
static Type *make_pointer();
static void make_direct_declarator(Node *node);
static Type *make_direct_decl_array();
static bool make_type_qual_list(bool *isc, bool *isv);
static List *make_parameter_type_list(bool *is_vargs);
static List *make_parameter_list();
static Node *make_parameter_decl();
static Type *ret_type_tail(Type *t);

static Node *make_primary_exp();
static Node *make_assignment_exp();
static Node *make_exp();

static Node *malloc_node(enum AST ast);
static Type *malloc_type(enum TypeType tt);
static Type *copy_type(const Type *t);
static void  pushback(Token *tk);
static Token *next();
static bool  is_puncid(const Token *tk, enum PnctID id);
static bool  is_keyword(const Token *tk, const char *s);

void
init_parser(const char *file)
{
    open_file(file);
    stack = make_list();
}

// List<Node*>
List *
parse_top()
{
    List *list;
    Node *node;

    list = make_list();
    for (;;)
    {
        node = make_extdecl();
        if (node == NULL) return list;
        add_list(list, node);
    }
}

static Node *
make_extdecl()
{
    Node *node;

    node = malloc_node(AST_VAR_DECL);
    // function-definition/declaration
    make_decl_spec(node);
    make_declarator(node);

    return node;
}

static void
make_decl_spec(Node *node)
{
    Type *type = NULL;
    Type *type_temp = NULL;
    bool is_inline = false;
    bool is_const = false;
    bool is_volatile = false;

    for (;;)
    {
        if (make_storage_class_spec(node))
            continue;
        if ((type_temp = make_type_spec()) != NULL)
        {
            if (type != NULL)
            {
                // error
            }
            type = type_temp;
            continue;
        }
        if (make_type_qual(&is_const, &is_volatile))
            continue;
        if (make_func_spec())
        {
            is_inline = true;
            continue;
        }
        break;
    }

    if (type == NULL) type = int_t;
    if (is_const)     type->is_const = true;
    if (is_volatile)  type->is_volatile = true;

    if (is_inline)
    {
        /* only function definition or prototype */
        Type *t_f = malloc_type(TT_FUNCTION);
        t_f->is_inline = true;
        t_f->ret = type;
        node->type = t_f;
    }
    else
    {
        node->type = type;
    }
}

static bool
make_storage_class_spec(Node *node)
{
    Token *tk;
    char *p;
    bool ret;

    tk = next();
    if (tk->kind != TK_KEYWORD)
    {
        pushback(tk);
        return false;
    }

    p = tk->str->str;
    ret = false;
    if (strcmp("typedef", p) == 0)
    {
        node->sc = SC_TYPEDEF;
        ret = true;
    }
    else if (strcmp("extern", p) == 0)
    {
        node->sc = SC_EXTERN;
        ret = true;
    }
    else if (strcmp("static", p) == 0)
    {
        node->sc = SC_STATIC;
        ret = true;
    }

    if (ret) free_token(&tk); pushback(tk);
    return ret;
}

static Type *
make_type_spec()
{
    Token *tk;
    char *p;

    tk = next();
    if (tk->kind != TK_KEYWORD)
    {
        pushback(tk);
        return false;
    }

    p = tk->str->str;
    if (strcmp("void", p) == 0)
    {
        free_token(&tk);
        return copy_type(void_t);
    }
    else if (strcmp("char", p) == 0)
    {
        free_token(&tk);
        return copy_type(char_t);
    }
    else if (strcmp("uchar", p) == 0)
    {
        free_token(&tk);
        return copy_type(uchar_t);
    }
    else if (strcmp("short", p) == 0)
    {
        free_token(&tk);
        return copy_type(short_t);
    }
    else if (strcmp("ushort", p) == 0)
    {
        free_token(&tk);
        return copy_type(ushort_t);
    }
    else if (strcmp("int", p) == 0)
    {
        free_token(&tk);
        return copy_type(int_t);
    }
    else if (strcmp("uint", p) == 0)
    {
        free_token(&tk);
        return copy_type(uint_t);
    }
    else if (strcmp("long", p) == 0)
    {
        free_token(&tk);
        return copy_type(long_t);
    }
    else if (strcmp("ulong", p) == 0)
    {
        free_token(&tk);
        return copy_type(ulong_t);
    }
    else if (strcmp("float", p) == 0)
    {
        free_token(&tk);
        return copy_type(float_t);
    }
    else if (strcmp("double", p) == 0)
    {
        free_token(&tk);
        return copy_type(double_t);
    }
    else if (strcmp("bool", p) == 0)
    {
        free_token(&tk);
        return copy_type(bool_t);
    }
    else if (strcmp("struct", p) == 0)
    {
        free_token(&tk);
        return NULL;
    }
    else if (strcmp("union", p) == 0)
    {
        free_token(&tk);
        return NULL;
    }
    else if (strcmp("enum", p) == 0)
    {
        free_token(&tk);
        return NULL;
    }
    else if (strcmp("typedef", p) == 0)
    {
        free_token(&tk);
        return NULL;
    }

    pushback(tk);
    return NULL;
}

static bool
make_type_qual(bool *isconst, bool *isvolatile)
{
    Token *tk;

    tk = next();
    if (tk->kind != TK_KEYWORD)
    {
        pushback(tk);
        return false;
    }

    if (strcmp("const", tk->str->str) == 0)
    {
        *isconst = true;
        free_token(&tk);
        return true;
    }
    else if (strcmp("volatile", tk->str->str) == 0)
    {
        *isvolatile = true;
        free_token(&tk);
        return true;
    }
    pushback(tk);
    return false;
}

static bool
make_func_spec()
{
    Token *tk;

    tk = next();
    if (tk->kind != TK_KEYWORD)
    {
        pushback(tk);
        return false;
    }

    if (strcmp("inline", tk->str->str) == 0)
    {
        free_token(&tk);
        return true;
    }
    pushback(tk);
    return false;
}

static void
make_declarator(Node *node)
{
    Type *ptr_head, *temp;

    ptr_head = make_pointer();
    temp = node->type;
    node->type = NULL;

    make_direct_declarator(node);
    
    if (ptr_head == NULL)
    {
        Type *temp_tail = ret_type_tail(node->type);
        if (temp_tail == NULL) node->type = temp;
        else if (temp_tail->tt == TT_FUNCTION) temp_tail->ret = temp;
        else if (temp_tail->tt == TT_POINTER)  temp_tail->ptr = temp;
        else if (temp_tail->tt == TT_ARRAY)    temp_tail->base = temp;
    }
    else
    {
        Type *ptr_tail, *temp_tail;
        for (ptr_tail = ptr_head; ptr_tail->ptr != NULL;)
            ptr_tail = ptr_tail->ptr;
        temp_tail = ret_type_tail(node->type);
        if (temp_tail == NULL)
        {
            node->type = ptr_head;
            ptr_tail->ptr = temp;
        }
        else if (temp_tail->tt == TT_FUNCTION)
        {
            temp_tail->ret = ptr_head;
            ptr_tail->ptr = temp;
        }
        else if (temp_tail->tt == TT_POINTER)
        {
            temp_tail->ptr = ptr_head;
            ptr_tail->ptr = temp;
        }
        else if (temp_tail->tt == TT_ARRAY)
        {
            temp_tail->base = ptr_head;
            ptr_tail->ptr = temp;
        }
    }
}

static Type *
make_pointer()
{
    Token *tk;
    Type *type = NULL;
    Type *type_temp = NULL;
    bool is_const;
    bool is_volatile;
    
    for (;;)
    {
        is_const = false;
        is_volatile = false;
        
        tk = next();
        if (!is_puncid(tk, P_MULT))
        {
            pushback(tk);
            return type;
        }
        free_token(&tk);
        
        type = copy_type(ptr_t);
        // type-qualifier-list
        while (make_type_qual(&is_const, &is_volatile));
        
        if (is_const)    type->is_const = true;
        if (is_volatile) type->is_volatile = true;
        
        type->ptr = type_temp;
        type_temp = type;
    }
}

static void
make_direct_declarator(Node *node)
{
    Token *tk;

    tk = next();
    if (tk->kind == TK_IDENT)
    {
        node->var_name = new_string(tk->str);
        free_token(&tk);
    }
    else if (is_puncid(tk, P_PAREN_L))
    {
        make_declarator(node);
        free_token(&tk);
        tk = next();
        if (is_puncid(tk, P_PAREN_R))
        {
            free_token(&tk);
        }
        else
        {
            // error
        }
    }
    else
    {
        // error
    }

    tk = next();
    if (is_puncid(tk, P_PAREN_L))
    {
        Type *t, *temp_tail;
        bool is_vargs = false;
        t = malloc_type(TT_FUNCTION);
        t->args = make_parameter_type_list(&is_vargs);
        
        temp_tail = ret_type_tail(node->type);
        if (temp_tail == NULL) node->type = t;
        else if (temp_tail->tt == TT_FUNCTION) temp_tail->ret = t;
        else if (temp_tail->tt == TT_POINTER)  temp_tail->ptr = t;
        else if (temp_tail->tt == TT_ARRAY)    temp_tail->base = t;

        free_token(&tk);
        tk = next();
        if (is_puncid(tk, P_PAREN_R))
        {
            free_token(&tk);
            return;
        }
        else
        {
            // error
        }
    }
    else if (is_puncid(tk, P_SQR_BRCK_L))
    {
        Type *t, *temp_tail;
        t = make_direct_decl_array();

        temp_tail = ret_type_tail(node->type);
        if (temp_tail == NULL) node->type = t;
        else if (temp_tail->tt == TT_FUNCTION) temp_tail->ret = t;
        else if (temp_tail->tt == TT_POINTER)  temp_tail->ptr = t;
        else if (temp_tail->tt == TT_ARRAY)    temp_tail->base = t;

        free_token(&tk);
        tk = next();
        if (is_puncid(tk, P_SQR_BRCK_R))
        {
            free_token(&tk);
            return;
        }
        else
        {
            // error
        }
    }
    else
    {
        pushback(tk);
        return;
    }
}

static Type *
make_direct_decl_array()
{
    Token *tk;
    Type *t;
    bool isconst = false, isvolatile = false;
    t = malloc_type(TT_ARRAY);
    tk = next();
    if (is_keyword(tk, "static"))
    {
        free_token(&tk);
        make_type_qual_list(&isconst, &isvolatile);
        t->is_const = isconst;
        t->is_volatile = isvolatile;
        t->is_static = true;
        t->asn_exp = make_assignment_exp();
        if (t->asn_exp == NULL)
        {
            // error
        }
        return t;
    }
    else
    {
        pushback(tk);
        if (make_type_qual_list(&isconst, &isvolatile))
        {
            t->is_const = isconst;
            t->is_volatile = isvolatile;
            if (is_keyword(tk, "static"))
            {
                t->is_static = true;
                t->asn_exp = make_assignment_exp();
                if (t->asn_exp == NULL)
                {
                    //error
                }
                return t;
            }
        }
        tk = next();
        if (is_puncid(tk, P_MULT))
        {
            free_token(&tk);
            t->is_varg = true;
            return t;
        }
        else
        {
            pushback(tk);
            t->asn_exp = make_assignment_exp();
            return t;
        }
    }
}

static bool
make_type_qual_list(bool *is_const, bool *is_volatile)
{
    bool ret;

    ret = make_type_qual(is_const, is_volatile);
    if (!ret) return false;

    while (make_type_qual(is_const, is_volatile));
    return true;
}

static List *
make_parameter_type_list(bool *is_vargs)
{
    List *list;
    Token *tk, *tk2;

    list = make_parameter_list();
    
    tk = next();
    if (is_puncid(tk, P_COMMA))
    {
        tk2 = next();
        if (is_puncid(tk2, P_TLEAD))
        {
            free_token(&tk);
            free_token(&tk2);
            *is_vargs = true;
        }
        else
        {
            pushback(tk2);
            pushback(tk);
        }
    }
    else
    {
        pushback(tk);
    }
    return list;
}

static List *
make_parameter_list()
{
    Token *tk;
    List *list;
    Node *node;

    list = make_list();
    
    tk = next();
    if (is_puncid(tk, P_PAREN_R))
    {
        pushback(tk);
        return list;
    }
    else
    {
        pushback(tk);
    }
    for (;;)
    {
        node = make_parameter_decl();
        if (node == NULL) break;
        add_list(list, node);
        tk = next();
        if (is_puncid(tk, P_COMMA))
        {
            free_token(&tk);
        }
        else
        {
            pushback(tk);
            break;
        }
    }

    return list;
}

static Node *
make_parameter_decl()
{
    Node *node;
    node = malloc_node(AST_VAR_DECL);
    make_decl_spec(node);
    make_declarator(node);
    return node;
}

static Type *
ret_type_tail(Type *t)
{
    Type *temp_head;
    Type *temp_tail;

    temp_head = t;
    for (temp_tail = temp_head;;)
    {
        if (temp_tail == NULL) break;
        else if (temp_tail->tt == TT_FUNCTION)
        {
            if (temp_tail->ret == NULL) break;
            temp_tail = temp_tail->ret;
        }
        else if (temp_tail->tt == TT_POINTER)
        {
            if (temp_tail->ptr == NULL) break;
            temp_tail = temp_tail->ptr;
        }
        else if (temp_tail->tt == TT_ARRAY)
        {
            if (temp_tail->base == NULL) break;
            temp_tail = temp_tail->base;
        }
    }
    return temp_tail;
}

static Node *
make_primary_exp()
{
    Token *tk;
    Node *node;
    tk = next();
    if (tk->kind == TK_IDENT)
    {
        free_token(&tk);
        node = malloc_node(AST_IDENT);
    }
    else if (tk->kind == TK_CONSTANT)
    {
        free_token(&tk);
        node = malloc_node(AST_CONSTANT);
    }
    else if (tk->kind == TK_STRING)
    {
        free_token(&tk);
        node = malloc_node(AST_STRING);
    }
    else if (is_puncid(tk, P_PAREN_L))
    {
        free_token(&tk);
        Node *ret = make_exp();
        tk = next();
        if (is_puncid(tk, P_PAREN_R))
        {
            free_token(&tk);
            return ret;
        }
    }
    else
    {
        // error
    }
    return NULL;
}

static Node *
make_assignment_exp()
{
    return NULL;
}

static Node *
make_exp()
{
    return NULL;
}

static Node *
malloc_node(enum AST ast)
{
    Node *node;
    node = (Node*)malloc(sizeof(Node));
    node->kind = ast;

    switch (ast)
    {
    case AST_VAR_DECL:
        node->var_name = NULL;
        node->sc = SC_NONE;
        break;
    case AST_FUNC_DEF:
        node->func_name = NULL;
        node->body = NULL;
        break;
    }
    return node;
}

static Type *
malloc_type(enum TypeType tt)
{
    Type *t;
    t = (Type*)malloc(sizeof(Type));
    t->size = 0;
    t->align = 0;
    t->tt = tt;

    switch (tt)
    {
    case TT_BASIC:
        t->is_signed = false;
        break;
    case TT_STRUCT_UNION:
        t->is_struct = true;
        t->contents = NULL;
        break;
    case TT_ARRAY:
        t->is_static = false;
        t->asn_exp = false;
        break;
    case TT_FUNCTION:
        t->ret = NULL;
        t->args = NULL;
        t->is_vargs = false;
        t->is_inline = false;
        break;
    }
    return t;
}

static Type *
copy_type(const Type *t)
{
    Type *newt;
    
    newt = malloc_type(t->tt);
    newt->size = t->size;
    newt->align = t->align;
    newt->is_const = t->is_const;
    newt->is_volatile = t->is_volatile;
    newt->tt = t->tt;

    switch(t->tt)
    {
    case TT_BASIC:
        newt->is_signed = t->is_signed;
        break;
    case TT_STRUCT_UNION:
        newt->is_struct = t->is_struct;
        newt->contents = t->contents;
        break;
    case TT_ARRAY:
        newt->is_static = t->is_static;
        newt->asn_exp = t->asn_exp;
        newt->base = t->base;
        break;
    case TT_FUNCTION:
        newt->ret = t->ret;
        newt->args = t->args;
        newt->is_vargs = t->is_vargs;
        newt->is_inline = t->is_inline;
        break;
    case TT_POINTER:
        newt->ptr = NULL;
        break;
    }

    return newt;
}

static void
pushback(Token *tk) { add_list(stack, tk); }

static Token *
next()
{
    if (count_list(stack) != 0) return (Token*)pop_list(stack);
    return next_token();
}

static bool
is_puncid(const Token *tk, enum PnctID id)
{
    return tk->kind == TK_PUNCTUATOR && tk->id == id;
}

static bool
is_keyword(const Token *tk, const char *p)
{
    return tk->kind == TK_KEYWORD && strcmp(tk->str->str, p) == 0;
}

#ifdef TEST_LL_PARSER
int
main(int argc, char *argv[])
{
    init_parser(argv[1]);
    return EXIT_SUCCESS;
}
#endif

