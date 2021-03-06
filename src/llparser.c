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

static List *stack; // List<Token*>

/* prototypes */
// declaration
static void make_extdecl(List *list);
static Node *make_initializer();
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

// expression
static Node *make_primary_exp();
static Node *make_assignment_exp();
static Node *make_exp();
static Node *make_const_exp();

// statement
static Node *make_statement(bool in_switch);
static Node *make_labeled_stmt(bool in_switch);
static Node *make_compound_stmt(bool in_switch);
static Node *make_select_stmt(bool in_switch);
static Node *make_iteration_stmt(bool in_switch);
static Node *make_jump_stmt(bool in_switch);

// util
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
    Token *tk;

    list = make_list();
    for (;;)
    {
        make_extdecl(list);
        tk = next();
        if (tk->kind == TK_ENDFILE) return list;
    }
}

/* List<Node*> */
static void
make_extdecl(List *list)
{
    Node *node;
    Token *tk;

    node = malloc_node(AST_VAR_DECL);
    // function-definition/declaration
    make_decl_spec(node);
    make_declarator(node);

    tk = next();
    if (is_puncid(tk, P_CRL_BRCK_L))
    {
        // function definition
        free_token(&tk);
        node->kind = AST_FUNC_DEF;
        node->init_or_body = make_compound_stmt(false);
        tk = next();
        if (is_puncid(tk, P_CRL_BRCK_R))
        {
            free_token(&tk);
        }
        else
        {
            // error
        }
        add_list(list, node);
        return;
    }
    else
    {
        pushback(tk);
    }
    
    for (;;)
    {
        tk = next();
        if (is_puncid(tk, P_ASGN))
        {
            free_token(&tk);
            node->init_or_body = make_initializer();
        }
        else
        {
            pushback(tk);
        }
        
        tk = next();
        if (is_puncid(tk, P_COMMA))
        {
            Node *next_node;
            next_node = malloc_node(AST_VAR_DECL);
            next_node->sc = node->sc;
            next_node->type = ret_type_tail(node->type);
            make_declarator(next_node);
            add_list(list, node);
            node = next_node;
            free_token(&tk);
        }
        else if (is_puncid(tk, P_SCOLON))
        {
            free_token(&tk);
            add_list(list, node);
            return;
        }
        else
        {
            // error
        }
    }
}

static Node *
make_initializer()
{
    Token *tk;

    tk = next();
    if (is_puncid(tk, P_CRL_BRCK_L))
    {
        free_token(&tk);
        // TODO
    }
    else
    {
        pushback(tk);
        return make_assignment_exp();
    }
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
    bool ret;

    tk = next();
    if (tk->kind != TK_KEYWORD)
    {
        pushback(tk);
        return false;
    }

    ret = false;
    if (cmp2_string(tk->str, "typedef") == 0)
    {
        node->sc = SC_TYPEDEF;
        ret = true;
    }
    else if (cmp2_string(tk->str, "extern") == 0)
    {
        node->sc = SC_EXTERN;
        ret = true;
    }
    else if (cmp2_string(tk->str, "static") == 0)
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

    tk = next();
    if (tk->kind != TK_KEYWORD)
    {
        pushback(tk);
        return false;
    }


    if (cmp2_string(tk->str, "void") == 0)
    {
        free_token(&tk);
        return copy_type(void_t);
    }
    else if (cmp2_string(tk->str, "char") == 0)
    {
        free_token(&tk);
        return copy_type(char_t);
    }
    else if (cmp2_string(tk->str, "uchar") == 0)
    {
        free_token(&tk);
        return copy_type(uchar_t);
    }
    else if (cmp2_string(tk->str, "short") == 0)
    {
        free_token(&tk);
        return copy_type(short_t);
    }
    else if (cmp2_string(tk->str, "ushort") == 0)
    {
        free_token(&tk);
        return copy_type(ushort_t);
    }
    else if (cmp2_string(tk->str, "int") == 0)
    {
        free_token(&tk);
        return copy_type(int_t);
    }
    else if (cmp2_string(tk->str, "uint") == 0)
    {
        free_token(&tk);
        return copy_type(uint_t);
    }
    else if (cmp2_string(tk->str, "long") == 0)
    {
        free_token(&tk);
        return copy_type(long_t);
    }
    else if (cmp2_string(tk->str, "ulong") == 0)
    {
        free_token(&tk);
        return copy_type(ulong_t);
    }
    else if (cmp2_string(tk->str, "float") == 0)
    {
        free_token(&tk);
        return copy_type(float_t);
    }
    else if (cmp2_string(tk->str, "double") == 0)
    {
        free_token(&tk);
        return copy_type(double_t);
    }
    else if (cmp2_string(tk->str, "bool") == 0)
    {
        free_token(&tk);
        return copy_type(bool_t);
    }
    else if (cmp2_string(tk->str, "struct") == 0)
    {
        Type *struct_type;
        free_token(&tk);
        struct_type = malloc_type(TT_STRUCT_UNION);
        struct_type->is_struct = true;
        return struct_type;
    }
    else if (cmp2_string(tk->str, "union") == 0)
    {
        Type *union_type;
        free_token(&tk);
        union_type = malloc_type(TT_STRUCT_UNION);
        union_type->is_struct = false;
        return union_type;
    }
    else if (cmp2_string(tk->str, "enum") == 0)
    {
        free_token(&tk);
        return NULL;
    }
    else if (cmp2_string(tk->str, "typedef") == 0)
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

    if (cmp2_string(tk->str, "const") == 0)
    {
        *isconst = true;
        free_token(&tk);
        return true;
    }
    else if (cmp2_string(tk->str, "volatile") == 0)
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

    if (cmp2_string(tk->str, "inline") == 0)
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
        node->name = new_string(tk->str);
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
        pushback(tk);
        return;
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
        for (;;)
        {
            if (is_puncid(tk, P_SQR_BRCK_L))
            {
                free_token(&tk);
            }
            else
            {
                pushback(tk);
                return;
            }
            t = make_direct_decl_array();

            temp_tail = ret_type_tail(node->type);
            if (temp_tail == NULL) node->type = t;
            else if (temp_tail->tt == TT_FUNCTION) temp_tail->ret = t;
            else if (temp_tail->tt == TT_POINTER)  temp_tail->ptr = t;
            else if (temp_tail->tt == TT_ARRAY)    temp_tail->base = t;

            tk = next();
            if (is_puncid(tk, P_SQR_BRCK_R))
            {
                free_token(&tk);
            }
            else
            {
                // error
            }
            tk = next();
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
            t->is_varray = true;
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
        else
        {
            return t;
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
make_const_exp()
{
    return NULL;
}

/* statement */
static Node *
make_statement(bool in_switch)
{
    Node *node;

    node = make_labeled_stmt(in_switch);
    if (node != NULL) return node;

    node = make_compound_stmt(in_switch);
    if (node != NULL) return node;
    
    node = make_select_stmt(in_switch);
    if (node != NULL) return node;

    node = make_iteration_stmt(in_switch);
    if (node != NULL) return node;

    node = make_jump_stmt(in_switch);
    if (node != NULL) return node;

    // expression statement
    node = malloc_node(AST_EXP_STMT);
    node->exp_stmt = make_exp();
    return node;
}

static Node *
make_labeled_stmt(bool in_switch)
{
    Token *tk;
    Node *node = NULL;

    tk = next();
    if (in_switch)
    {
        // "case" and "default" are
        // appeared in only switch-statement
        if (is_keyword(tk, "case"))
        {
            free_token(&tk);
            node = malloc_node(AST_CASE);
            node->case_exp = make_const_exp();

            tk = next();
            if (is_puncid(tk, P_COLON))
            {
                free_token(&tk);
                node->case_stmt = make_statement(true);
                return node;
            }
            else
            {
                pushback(tk);
                // error
            }
        }
        else if (is_keyword(tk, "default"))
        {
            free_token(&tk);
            node = malloc_node(AST_DEFAULT);

            tk = next();
            if (is_puncid(tk, P_COLON))
            {
                free_token(&tk);
                node->default_stmt = make_statement(true);
                return node;
            }
            else
            {
                pushback(tk);
                // error
            }
        }
    }
    if (tk->kind == TK_IDENT)
    {
        Token *tk2 = next();
        if (is_puncid(tk2, P_COLON))
        {
            node = malloc_node(AST_LABEL);
            node->label = new_string(tk->str);
            node->label_stmt = make_statement(in_switch);
            free_token(&tk);
            free_token(&tk2);
            return node;
        }
        else
        {
            pushback(tk);
            pushback(tk2);
        }
    }
    else
    {
        pushback(tk);
    }
    return NULL;
}

static Node *
make_compound_stmt(bool in_switch)
{
    Token *tk;

    tk = next();
    if (is_puncid(tk, P_CRL_BRCK_L))
    {
        Node *node = NULL;
        List *list = make_list();
        node = malloc_node(AST_COMP_STMT);
        node->stmts = list;

        free_token(&tk);
        for (;;)
        {
            tk = next();
            if (is_puncid(tk, P_CRL_BRCK_R))
            {
                free_token(&tk);
                return node;
            }
            else
            {
                pushback(tk);
            }
            // TODO
        }
    }
    return NULL;
}

static Node *
make_select_stmt(bool in_switch)
{
    Token *tk;
    Node *node = NULL;

    tk = next();
    if (is_keyword(tk, "if"))
    {
        free_token(&tk);

        tk = next();
        if (is_puncid(tk, P_PAREN_L))
        {
            free_token(&tk);
            node = malloc_node(AST_IF);
            node->if_exp = make_exp();

            tk = next();
            if (is_puncid(tk, P_PAREN_R))
            {
                free_token(&tk);
            }
            else
            {
                pushback(tk);
                // error
            }

            node->true_stmt = make_statement(in_switch);

            tk = next();
            if (is_keyword(tk, "else"))
            {
                free_token(&tk);
                node->false_stmt = make_statement(in_switch);
            }
            else
            {
                pushback(tk);
            }
            return node;
        }
        else
        {
            pushback(tk);
            // error
        }
    }
    else if (is_keyword(tk, "switch"))
    {
        free_token(&tk);
        node = malloc_node(AST_SWITCH);

        tk = next();
        if (is_puncid(tk, P_PAREN_L))
        {
            free_token(&tk);
            node->switch_cond = make_exp();

            tk = next();
            if (is_puncid(tk, P_PAREN_R))
            {
                free_token(&tk);
                node->switch_stmt = make_statement(true);
                return node;
            }
            else
            {
                pushback(tk);
                // error
            }
        }
        else
        {
            pushback(tk);
            // error
        }
    }
    else
    {
        pushback(tk);
    }
    return NULL;
}

static Node *
make_iteration_stmt(bool in_switch)
{
    Token *tk;
    Node *node;

    tk = next();
    if (is_keyword(tk, "while"))
    {
        free_token(&tk);
        node = malloc_node(AST_WHILE);

        tk = next();
        if (is_puncid(tk, P_PAREN_L))
        {
            free_token(&tk);
            node->while_cond = make_exp();

            tk = next();
            if (is_puncid(tk, P_PAREN_R))
            {
                free_token(&tk);
                node->while_body = make_statement(in_switch);
                return node;
            }
            else
            {
                pushback(tk);
                // error
            }
        }
        else
        {
            pushback(tk);
            // error
        }
    }
    else if (is_keyword(tk, "do"))
    {
        free_token(&tk);
        node = malloc_node(AST_DO);
        node->do_body = make_statement(in_switch);

        tk = next();
        if (is_keyword(tk, "while"))
        {
            free_token(&tk);
            tk = next();
            if (is_puncid(tk, P_PAREN_L))
            {
                free_token(&tk);
                node->do_body = make_exp();
                tk = next();
                if (is_puncid(tk, P_PAREN_R))
                {
                    free_token(&tk);
                    tk = next();
                    if (is_puncid(tk, P_SCOLON))
                    {
                        free_token(&tk);
                        return node;
                    }
                    else
                    {
                        pushback(tk);
                        // error
                    }
                }
                else
                {
                    pushback(tk);
                    // error
                }
            }
            else
            {
                pushback(tk);
                // error
            }
        }
        else
        {
            pushback(tk);
            // error
        }
    }
    else if (is_keyword(tk, "for"))
    {
        free_token(&tk);
        node = malloc_node(AST_FOR);
        tk = next();
        if (is_puncid(tk, P_PAREN_L))
        {
            free_token(&tk);
            tk = next();
            if (is_puncid(tk, P_SCOLON))
            {
                free_token(&tk);
            }
            else
            {
                pushback(tk);
                node->for_init_exp = make_exp();
                tk = next();
                if (is_puncid(tk, P_SCOLON))
                {
                    free_token(&tk);
                }
                else
                {
                    pushback(tk);
                    // error
                }
            }
            tk = next();
            if (is_puncid(tk, P_SCOLON))
            {
                free_token(&tk);
            }
            else
            {
                pushback(tk);
                node->for_cond_exp = make_exp();
                tk = next();
                if (is_puncid(tk, P_SCOLON))
                {
                    free_token(&tk);
                }
                else
                {
                    pushback(tk);
                    // error
                }
            }
            node->for_step_exp = make_exp();
            tk = next();
            if (is_puncid(tk, P_PAREN_R))
            {
                free_token(&tk);
                node->for_body = make_statement(in_switch);
                return node;
            }
            else
            {
                pushback(tk);
                // error
            }
        }
        else
        {
            pushback(tk);
            // error
        }
    }
    else
    {
        pushback(tk);
    }
    return NULL;
}

static Node *
make_jump_stmt(bool in_switch)
{
    Token *tk;
    Node *node;
    
    tk = next();
    if (is_keyword(tk, "goto"))
    {
        free_token(&tk);
        node = malloc_node(AST_GOTO);
        tk = next();
        if (tk->kind == TK_IDENT)
        {
            node->goto_label = new_string(tk->str);
            free_token(&tk);
            return node;
        }
        else
        {
            pushback(tk);
            // error
        }
    }
    else if (is_keyword(tk, "continue"))
    {
        free_token(&tk);
        node = malloc_node(AST_CONTINUE);
        return node;
    }
    else if (is_keyword(tk, "break"))
    {
        free_token(&tk);
        node = malloc_node(AST_BREAK);
        return node;
    }
    else if (is_keyword(tk, "return"))
    {
        free_token(&tk);
        node = malloc_node(AST_RETURN);
        tk = next();
        if (is_puncid(tk, P_SCOLON))
        {
            free_token(&tk);
        }
        else
        {
            pushback(tk);
            node->return_exp = make_expression();
        }
        return node;
    }
    else
    {
        pushback(tk);
    }

    return NULL;
}

static Node *
malloc_node(enum AST ast)
{
    Node *node;
    node = (Node*)try_calloc(1, sizeof(Node));
    node->kind = ast;

    switch (ast)
    {
    case AST_VAR_DECL:
        node->name = NULL;
        node->init_or_body = NULL;
        node->sc = SC_NONE;
        break;
    case AST_FUNC_DEF:
        node->name = NULL;
        node->init_or_body = NULL;
        node->sc = SC_NONE;
        break;
    case AST_LABEL:
        node->label = NULL;
        node->label_stmt = NULL;
        break;
    case AST_CASE:
        node->case_exp = NULL;
        node->case_stmt = NULL;
        break;
    case AST_DEFAULT:
        node->default_stmt = NULL;
        break;
    case AST_COMP_STMT:
        node->stmts = NULL;
        break;
    case AST_EXP_STMT:
        node->exp_stmt = NULL;
        break;
    case AST_IF:
        node->if_exp = NULL;
        node->true_stmt = NULL;
        node->false_stmt = NULL;
        break;
    case AST_SWITCH:
        node->switch_cond = NULL;
        node->switch_stmt = NULL;
        break;
    case AST_WHILE:
        node->while_cond = NULL;
        node->while_body = NULL;
        break;
    case AST_DO:
        node->do_cond = NULL;
        node->do_body = NULL;
        break;
    case AST_FOR:
        node->for_init_exp = NULL;
        node->for_cond_exp = NULL;
        node->for_step_exp = NULL;
        node->for_body = NULL;
        break;
    case AST_GOTO:
        node->goto_label = NULL;
        break;
    case AST_RETURN:
        node->return_exp = NULL;
        break;
    }
    return node;
}

static Type *
malloc_type(enum TypeType tt)
{
    Type *t;
    t = (Type*)try_malloc(sizeof(Type));
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
    case TT_FUNCTION:
        t->ret = NULL;
        t->args = NULL;
        t->is_vargs = false;
        t->is_inline = false;
        break;
    case TT_ARRAY:
        t->is_static = false;
        t->asn_exp = false;
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
    case TT_FUNCTION:
        newt->ret = t->ret;
        newt->args = t->args;
        newt->is_vargs = t->is_vargs;
        newt->is_inline = t->is_inline;
        break;
    case TT_ARRAY:
        newt->is_static = t->is_static;
        newt->is_varray = t->is_varray;
        newt->asn_exp = t->asn_exp;
        newt->base = t->base;
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
    if (stack->len != 0) return (Token*)pop_list(stack);
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
    return tk->kind == TK_KEYWORD && cmp2_string(tk->str, p) == 0;
}

#ifdef TEST_LL_PARSER
#define ARRAY_LEN(x) (sizeof((x))/sizeof(char))
static void output_init(FILE *f);
static void output_end(FILE *f);
static void output_node(FILE *f, const Node *node, const char *parent, const char *edge);
static void output_type(FILE *f, const Type *type, const char *parent, const char *edge);

static void
output_init(FILE *f) { fprintf(f, "digraph {\n"); }
static void
output_end(FILE *f) { fprintf(f, "}\n"); }

static void
output_node(FILE *f, const Node *node, const char *parent, const char *edge)
{
    switch (node->kind)
    {
    case AST_VAR_DECL:
    case AST_FUNC_DEF:
        {
            char *p = NULL, *t = NULL, str[256];
            snprintf(str, ARRAY_LEN(str), "node_%s", node->name->str);

            if (node->sc == SC_NONE)         p = "NONE";
            else if (node->sc == SC_TYPEDEF) p = "TYPEDEF";
            else if (node->sc == SC_EXTERN)  p = "EXTERN";
            else if (node->sc == SC_STATIC)  p = "STATIC";

            if (node->kind == AST_VAR_DECL)      t = "VARDECL";
            else if (node->kind == AST_FUNC_DEF) t = "FUNCDEF";

            fprintf(f, "%s [shape=box, label=\"%s\\nname=%s\\nsc=%s\"];\n",
                    str, t, node->name->str, p);
            if (parent != NULL)
            {
                fprintf(f, "%s -> %s", parent, str);
                if (edge != NULL)
                {
                    fprintf(f, " [label=\"%s\"];", edge);
                }
                fprintf(f, "\n");
            }
            output_type(f, node->type, str, "Type");
        }
        break;
    default:
        break;
    }
}

static void
output_type(FILE *f, const Type *type, const char *parent, const char *edge)
{
    static unsigned int type_count = 0;
    char p[256];
    if (type == NULL) return;

    snprintf(p, ARRAY_LEN(p), "type_%d", type_count++);
    switch (type->tt)
    {
    case TT_BASIC:
        {
            fprintf(f, "%s [shape=box, label=\"%s,%d,%d\\nconst=%d\\nvolatile=%d\\nsigned=%d\"];\n",
                    p, "BASIC", type->size, type->align,
                    type->is_const, type->is_volatile, type->is_signed);
        }
        break;
    case TT_STRUCT_UNION:
        {
            fprintf(f, "%s [shape=box, label=\"%s,%d,%d\\nconst=%d\\nvolatile=%d\\n\"];\n",
                    p, type->is_struct ? "STRUCT" : "UNION", type->size, type->align,
                    type->is_const, type->is_volatile);
        }
        break;
    case TT_POINTER:
        {
            fprintf(f, "%s [shape=box, label=\"%s,%d,%d\\nconst=%d\\nvolatile=%d\\n\"];\n",
                    p, "POINTER", type->size, type->align,
                    type->is_const, type->is_volatile);
            output_type(f, type->ptr, p, "ptr");
        }
        break;
    case TT_ARRAY:
        {
            fprintf(f, "%s [shape=box, label=\"%s,%d,%d\\nconst=%d\\nvolatile=%d\\nstatic=%d\\nvarg=%d\\n\"];\n",
                    p, "ARRAY", type->size, type->align,
                    type->is_const, type->is_volatile, type->is_static, type->is_varray);
            output_type(f, type->base, p, "base");
        }
        break;
    case TT_FUNCTION:
        {
            iter_list iter;
            fprintf(f, "%s [shape=box, label=\"%s,%d,%d\\nconst=%d\\nvolatile=%d\\nvargs=%d\\ninline=%d\"];\n",
                    p, "FUNCTION", type->size, type->align,
                    type->is_const, type->is_volatile, type->is_vargs, type->is_inline);
            output_type(f, type->ret, p, "ret");
            if (type->args == NULL) break;
            for (init_iter_list(type->args, &iter);
                 hasnext_iter_list(&iter);
                 next_iter_list(&iter))
            {
                output_node(f, ((Node*)value_iter_list(&iter)), p, "args");
            }
        }
        break;
    }
    fprintf(f, "%s -> %s [label=\"%s\"];\n", parent, p, edge);
}

int
main(int argc, char *argv[])
{
    List *list;
    FILE *file;
    char output[256];
    iter_list iter;

    init_parser(argv[1]);
    list = parse_top();

    snprintf(output, sizeof(output)/sizeof(char), "%s.dot", argv[1]);
    file = fopen(output, "w");
    {
        output_init(file);

        for (init_iter_list(list, &iter);
                hasnext_iter_list(&iter);
                next_iter_list(&iter))
        {
            output_node(file, (Node*)value_iter_list(&iter), NULL, NULL);
        }

    } output_end(file);

    fclose(file);
    return EXIT_SUCCESS;
}
#endif

