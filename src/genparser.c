#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tilde.h"

// To change depending on the size of the grammar.
#define RULE_SIZE (256)

enum BNFKind
{
    NONTERM,
    TERM,
    END,
};

typedef struct
{
    enum BNFKind kind;
    String *str;
} Factor;

typedef struct
{
    List *rule; // List<Factor*>
    Factor *lookahed;
    // "dot" is a point of interest.
    // For example, if following LR1-term exist, 
    // "@" is dot.
    //   [E -> @ E + T, EOF]
    // At this time, variable "dot" is equal to 0.
    //   [E -> E @ + T, EOF]
    // "dot" is equal to 1.
    int dot;
} LR1term;

// Hashmap<char*,Factor*>
static Hashmap *hashmap;
// All NON-TERM, TERM and END factors are stored.
static Factor factor[RULE_SIZE];
// Syntax
static List *rules[RULE_SIZE];

static Factor start_rule;
static Factor end_rule;

static int factor_idx;
static int rules_idx;

/* prototypes */
static void    parse();
static void    parse2(Factor *lkeyword);
static void    print_rules();
static Factor *find_initial_state();
static void    liberator_void(void*);
static void    calc_canonical_automaton();
static List   *calc_closure(LR1term *term);
static void    calc_closure_rec(List *set, LR1term *term);
static LR1term*make_LR1term(List *rule, Factor *la, int d);
static Factor *get_dot_factor(const LR1term *term);
static void    print_LR1set(List *set);

static void
parse()
{
    Token tk;
    Factor *f;

    for (;;)
    {
        next_token(&tk);
        if (tk.kind == TK_IDENT)
        {
            f = &factor[factor_idx];
            f->kind = NONTERM;
            f->str = tk.str;
            if (add_hashmap(hashmap, tk.str, f))
            {
                factor_idx++;
            }
            else
            {
                f = search_hashmap(hashmap, tk.str);
            }
        }
        else if (tk.kind == TK_ENDFILE)
        {
            return;
        }
        else
        {
            printf("Error:%d:%d: The beginning of the grammar rules must be an identifier.\n", tk.row, tk.col);
            exit(EXIT_FAILURE);
        }

        next_token(&tk);
        if (!(tk.kind == TK_PUNCTUATOR && tk.id == P_COLON))
        {
            printf("Error:%d:%d: Following the first non-terminal symbol of the grammar rules must be a colon.\n", tk.row, tk.col);
            exit(EXIT_FAILURE);
        }

        parse2(f);
    }
}

static void
parse2(Factor *lkeyword)
{
    Token tk;
    Factor *f;
    List *new_rule;

    new_rule = make_list();
    add_list(new_rule, lkeyword);
    for (;;)
    {
        next_token(&tk);
        switch (tk.kind)
        {
        case TK_IDENT:
            f = &factor[factor_idx];
            f->kind = NONTERM;
            f->str = tk.str;
            if (add_hashmap(hashmap, tk.str, f))
            {
                factor_idx++;
                add_list(new_rule, f);
            }
            else
            {
                add_list(new_rule, search_hashmap(hashmap, tk.str));
            }
            break;
        case TK_STRING:
            f = &factor[factor_idx];
            f->kind = TERM;
            f->str = tk.str;
            if (add_hashmap(hashmap, tk.str, f))
            {
                factor_idx++;
                add_list(new_rule, f);
            }
            else
            {
                add_list(new_rule, search_hashmap(hashmap, tk.str));
            }
            break;
        case TK_PUNCTUATOR:
            switch (tk.id)
            {
                case P_VBAR:
                    rules[rules_idx++] = new_rule;
                    new_rule = make_list();
                    add_list(new_rule, lkeyword);
                    break;
                case P_SCOLON:
                    rules[rules_idx++] = new_rule;
                    return;
                case P_PLUS:
                    next_token(&tk);
                    if (strcmp(tk.str->str, "token") == 0)
                    {
                        ((Factor*)at_list(new_rule, count_list(new_rule)-1))->kind = TERM;
                    }
                    else
                    {
                        printf("Error:%d:%d: Invalid token option.\n", tk.row, tk.col);
                        exit(EXIT_FAILURE);
                    }
                    break;
                default:
                    printf("Error:%d:%d: Invalid punctuator token.\n", tk.row, tk.col);
                    exit(EXIT_FAILURE);
            }
            break;
        default:
            printf("Error:%d:%d: Invalid token.\n", tk.row, tk.col);
            exit(EXIT_FAILURE);
        }
    }
}

static void
print_rules()
{
    iter_list iter;
    Factor *f;
    int i;

    for (i = 0; rules[i] != NULL; ++i)
    {
        init_iter_list(rules[i], &iter);
        printf("%3d:%s : ", i, ((Factor*)value_iter_list(&iter))->str->str);
        next_iter_list(&iter);
        for ( ; hasnext_iter_list(&iter); next_iter_list(&iter))
        {
            f = (Factor*)value_iter_list(&iter);
            printf("%s(%s) ", f->str->str, f->kind == TERM ? "TERM" : "NONTERM");
        }
        printf("\n");
    }
}

static Factor *
find_initial_state()
{
    Hashmap *hash;
    iter_list iter;
    Factor *f;
    char temp_flag[RULE_SIZE];
    int temp_flag_idx, i, ret;
    String *left;

    hash = make_hashmap(RULE_SIZE*2);
    temp_flag_idx = 0;
    ret = -1;

    for (i = 0; i < factor_idx; ++i)
    {
        f = factor+i;
        if (f->kind == NONTERM)
        {
            temp_flag[temp_flag_idx] = 0;
            add_hashmap(hash, f->str, temp_flag+temp_flag_idx);
        }
        else
        {
            temp_flag[temp_flag_idx] = -1;
        }
        temp_flag_idx++;
    }

    for (i = 0; rules[i] != NULL; ++i)
    {
        init_iter_list(rules[i], &iter);
        left = ((Factor*)value_iter_list(&iter))->str;
        next_iter_list(&iter);
        for (; hasnext_iter_list(&iter); next_iter_list(&iter))
        {
            f = (Factor*)value_iter_list(&iter);
            if (f->kind == NONTERM && strcmp(f->str->str, left->str) != 0)
            {
                *((char*)search_hashmap(hash, f->str)) = 1;
            }
        }
    }

    for (i = 0; i < factor_idx; ++i)
    {
        f = factor+i;
        if (f->kind == NONTERM)
        {
            if (*((char*)search_hashmap(hash, f->str)) == 0)
            {
                if (ret < 0)
                {
                    ret = i;
                }
                else
                {
                    fprintf(stderr, "Error: Syntax have two or more initial state(%s, %s).\n", factor[ret].str->str, factor[i].str->str);
                    exit(EXIT_FAILURE);
                }
            }
        }
    }

    if (ret < 0)
    {
        fprintf(stderr, "Error: Syntax don't have any initial state.\n");
        exit(EXIT_FAILURE);
    }
    free_hashmap(&hash, liberator_void);
    return &factor[ret];
}

static void
liberator_void(void *data){}

static void
calc_canonical_automaton()
{
    Factor *initialstate = find_initial_state();
    List *initrule;
    LR1term *term;

    printf("\nInitial state: %s\n", initialstate->str->str);

    initrule = make_list();
    add_list(initrule, &start_rule);
    add_list(initrule, initialstate);

    term = make_LR1term(initrule, &end_rule, 0);

    puts("");
    print_LR1set(calc_closure(term));
}

// List<LR1term*>
static List*
calc_closure(LR1term *term)
{
    List *set;

    set = make_list();
    add_list(set, term);
    calc_closure_rec(set, term);

    return set;
}

static void
calc_closure_rec(List *set, LR1term *term)
{
    Factor *lkeyword;
    Factor *dotfactor;
    Factor *new_dotfactor;
    LR1term *new_lr1term;
    int i;

    dotfactor = get_dot_factor(term);
    if (dotfactor != NULL && dotfactor->kind != NONTERM) return;

    for (i = 0; rules[i] != NULL; ++i)
    {
        lkeyword = (Factor*)at_list(rules[i], 0);
        if (strcmp(lkeyword->str->str, dotfactor->str->str) == 0)
        {
            new_lr1term = make_LR1term(rules[i], term->lookahed, term->dot);
            add_list(set, new_lr1term);
            new_dotfactor = get_dot_factor(new_lr1term);
            if (new_dotfactor != NULL && strcmp(new_dotfactor->str->str,
                       ((Factor*)at_list(new_lr1term->rule, 0))->str->str) != 0)
            {
                calc_closure_rec(set, new_lr1term);
            }
        }
    }
}

static LR1term *
make_LR1term(List *rule, Factor *la, int d)
{
    LR1term *lr1term;
    
    lr1term = (LR1term*)malloc(sizeof(LR1term));
    lr1term->rule = rule;
    lr1term->lookahed = la;
    lr1term->dot = d;
    return lr1term;
}

static Factor *
get_dot_factor(const LR1term *term)
{
    return (Factor*)at_list(term->rule, term->dot+1);
}

static void
print_LR1set(List *set)
{
    iter_list iter, iter2;
    LR1term *term;
    int i;
    int dot;

    for (init_iter_list(set, &iter);
         hasnext_iter_list(&iter);
         next_iter_list(&iter))
    {
        term = (LR1term*)value_iter_list(&iter);
        
        init_iter_list(term->rule, &iter2);
        printf("[%s -> ", ((Factor*)value_iter_list(&iter2))->str->str);
        next_iter_list(&iter2);
        for (i = 0, dot = term->dot; hasnext_iter_list(&iter2); next_iter_list(&iter2), ++i)
        {
            if (i == dot) printf(" @ ");
            printf("%s ", ((Factor*)value_iter_list(&iter2))->str->str);
        }
        printf(", %s]\n", term->lookahed->str->str);
    }
}

int
main(int argc, char *argv[])
{
    int i;

    if (argc != 2) exit(EXIT_FAILURE);
    if (!open_file(argv[1], 1)) exit(EXIT_FAILURE);

    hashmap = make_hashmap(RULE_SIZE*2);
    start_rule.kind = NONTERM;
    start_rule.str = new2_string("0S");
    end_rule.kind = END;
    end_rule.str = new2_string("$END");
    for (i = 0; i < RULE_SIZE; ++i) rules[i] = NULL;
    factor_idx = 0;
    rules_idx = 0;

    parse();
    print_rules();
    calc_canonical_automaton();

    return 0;
}

