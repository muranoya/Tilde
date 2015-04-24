#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "tilde.h"

#define STACK_SIZE (8)

static FILE *file = NULL;
static int line, column;

static int char_stack[STACK_SIZE], sp;

static char *keywords[] =
{
    "enum",   "break",   "return", "void",   "case",     "float",
    "short",  "ushort",  "char",   "uchar",  "for",      "while",
    "const",  "goto",    "sizeof", "bool",   "continue", "if",
    "static", "default", "inline", "struct", "do",       "int",
    "uint",   "switch",  "double", "long",   "ulong",    "typedef",
    "else",   "union",   "extern", "true",   "false",    "volatile",
    NULL,
};

static Token *endtoken = &(Token){.kind = TK_ENDFILE, .str = NULL};

/* prototype */
static bool make_ident(Token *tk, int c);
static bool make_digit(Token *tk, int c);
static bool make_digit_base(int c, int base, String *p);
static bool make_float_base(int c, int base, String *p);
static bool make_string_literal(Token *tk);
static bool make_punctuator(Token *tk, int c);

static bool estimate(int x);
static bool is_simple_escape(int c);
static bool is_nondigit(int c);
static bool is_digit(int c, int base);
static bool is_space(int c);
static bool is_return(int c);

static void set_keyword(Token *tk);
static void skip();
static void pushback(int c);
static int  nextchar();

bool
open_file(const char *f)
{
    file = fopen(f, "r");
    if (file == NULL)
    {
        perror("fopen");
        return false;
    }
    line = 1;
    column = 0;
    sp = 0;
    return true;
}

void
close_file()
{
    if (file == NULL) return;
    if (fclose(file) != 0) perror("fclose");
    file = NULL;
}

void
print_token(const Token *tk)
{
    char *p;
    char *s;
    switch (tk->kind)
    {
    case TK_IDENT:      p = "IDENT";      break;
    case TK_KEYWORD:    p = "KEYWORD";    break;
    case TK_CONSTANT:   p = "CONSTANT";   break;
    case TK_STRING:     p = "STRING";     break;
    case TK_PUNCTUATOR: p = "PUNCTUATOR"; break;
    case TK_ENDFILE:    p = "ENDFILE";    break;
    default:            p = "UNKNOWN";    break;
    }

    if (tk->kind == TK_PUNCTUATOR)   s = punctuator_to_string(tk->id);
    else if (tk->kind == TK_ENDFILE) s = "";
    else                             s = tk->str->str;
    printf("%s (%d:%d)%s\n",
            p, tk->row, tk->col, s);
}

char *
punctuator_to_string(enum PnctID p)
{
    switch (p)
    {
    case P_SQR_BRCK_L:   return "[";
    case P_SQR_BRCK_R:   return "]";
    case P_PAREN_L:      return "(";
    case P_PAREN_R:      return ")";
    case P_CRL_BRCK_L:   return "{";
    case P_CRL_BRCK_R:   return "}";
    case P_DOT:          return ".";
    case P_ARRW:         return "->";
    case P_PLUS_PLUS:    return "++";
    case P_MINS_MINS:    return "--";
    case P_AMPD:         return "&";
    case P_MULT:         return "*";
    case P_PLUS:         return "+";
    case P_MINS:         return "-";
    case P_TILDE:        return "~";
    case P_EXCM:         return "!";
    case P_DIV:          return "/";
    case P_MOD:          return "%";
    case P_SHIFT_L:      return "<<";
    case P_SHIFT_R:      return ">>";
    case P_LESS:         return "<";
    case P_GRT:          return ">";
    case P_LESS_EQ:      return "<=";
    case P_GRT_EQ:       return ">=";
    case P_EQ:           return "==";
    case P_NEQ:          return "!=";
    case P_CARET:        return "^";
    case P_VBAR:         return "|";
    case P_AMPD_AMPD:    return "&&";
    case P_VBAR_VBAR:    return "||";
    case P_QMARK:        return "?";
    case P_COLON:        return ":";
    case P_SCOLON:       return ";";
    case P_TLEAD:        return "...";
    case P_COMMA:        return ",";
    case P_ASGN:         return "=";
    case P_ASGN_MULT:    return "*=";
    case P_ASGN_DIV:     return "/=";
    case P_ASGN_MOD:     return "%=";
    case P_ASGN_PLUS:    return "+=";
    case P_ASGN_MINS:    return "-=";
    case P_ASGN_SHIFT_L: return "<<=";
    case P_ASGN_SHIFT_R: return ">>=";
    case P_ASGN_AMPD:    return "&=";
    case P_ASGN_CARET:   return "^=";
    case P_ASGN_VBAR:    return "|=";
    default:             return "UNKNOWN";
    }
}

Token *
next_token()
{
    Token *token;
    int c;
    skip();
    token = (Token*)try_malloc(sizeof(Token));
    c = nextchar();

    token->kind = TK_INVALID;
    token->id = P_INVALID;
    token->row = line;
    token->col = column;
    token->str = NULL;

    if (is_nondigit(c))
    {
        make_ident(token, c);
        return token;
    }
    else if (is_digit(c, 10))
    {
        make_digit(token, c);
        return token;
    }
    else if (c == '"')
    {
        make_string_literal(token);
        return token;
    }
    else if (c == '\'')
    {
        String *p = make_string();
        token->kind = TK_CONSTANT;
        token->row = line;
        token->col = column-1;
        token->str = p;

        c = nextchar();
        if (c == '\\')
        {
            int d = nextchar();
            if (is_simple_escape(d))
            {
                append2_string(p, '\\');
                append2_string(p, d);
            }
            else
            {
                print_error(token->row, token->col,
                        "Invalid escape character \"%c\".", (char)d);
            }
        }
        else
        {
            append2_string(p, c);
        }
        if (!estimate('\''))
        {
            print_error(token->row, token->col,
                    "Single quotation is required.");
        }
        return token;
    }
    else if (c == '.')
    {
        c = nextchar();
        if (is_digit(c, 10))
        {
            String *p = make_string();
            pushback(c);
            token->kind = TK_CONSTANT;
            token->row = line;
            token->col = column-1;
            token->str = p;
            make_float_base('.', 10, p);
            return token;
        }
        else
        {
            pushback(c);
        }
    }
    else if (c == EOF)
    {
        endtoken->row = line;
        endtoken->col = column;
        free_token(&token);
        return endtoken;
    }
    
    if (make_punctuator(token, c))
    {
        return token;
    }
    else
    {
        pushback(c);
    }

    print_error(line, column, "Invalid token.");
    exit(EXIT_FAILURE);
}

void
free_token(Token **tk)
{
    if ((*tk)->kind == TK_ENDFILE) return;
    free_string(&(*tk)->str);
    free(*tk);
    *tk = NULL;
}

static bool
make_ident(Token *tk, int c)
{
    String *ident = make_string();
    tk->kind = TK_IDENT;
    tk->row = line;
    tk->col = column;
    tk->str = ident;
    for (; is_nondigit(c) || is_digit(c, 10); c = nextchar())
    {
        append2_string(ident, c);
    }
    pushback(c);
    set_keyword(tk);
    return true;
}

static bool
make_digit(Token *tk, int c)
{
    tk->kind = TK_CONSTANT;
    tk->row = line;
    tk->col = column;
    tk->str = make_string();
    if (c == '0')
    {
        c = nextchar();
        if (c == 'x' || c == 'X')
        {
            append3_string(tk->str, "0x");
            return make_digit_base(nextchar(), 16, tk->str);
        }
        else
        {
            pushback(c);
            c = '0';
        }
    }
    return make_digit_base(c, 10, tk->str);
}

static bool
make_digit_base(int c, int base, String *p)
{
    for (; is_digit(c, base); c = nextchar())
    {
        append2_string(p, c);
    }

    if (c == '.' || c == 'e' || c == 'E' || c == 'p' || c == 'P')
    {
        return make_float_base(c, (base == 16) ? 16 : 10, p);
    }

    if (c == 'u' || c == 'U' || c == 'l' || c == 'L')
    {
        int d = nextchar();
        append2_string(p, c);
        if (((c == 'u' || c == 'U') && (d == 'l' || d == 'L')) ||
            ((c == 'l' || c == 'L') && (d == 'u' || d == 'U')))
        {
            append2_string(p, d);
        }
        else
        {
            pushback(d);
        }
    }
    else
    {
        pushback(c);
    }
    return true;
}

static bool
make_float_base(int c, int base, String *p)
{
    if (!(base == 10 && (c == '.' || c == 'e' || c == 'E')) &&
        !(base == 16 && (c == '.' || c == 'p' || c == 'P')))
    {
        //error
        return false;
    }

    if (c == '.')
    {
        append2_string(p, c);
        c = nextchar();
        if (is_digit(c, base))
        {
            for (; is_digit(c, base); c = nextchar())
            {
                append2_string(p, c);
            }
        }
    }
    else if (base == 10)
    {
        if (c != 'e' && c != 'E')
        {
            //error
        }
    }

    if (base == 16 && c != 'p' && c != 'P')
    {
        //error
    }

    if (c == 'e' || c == 'E' || c == 'p' || c == 'P')
    {
        append2_string(p, c);
        c = nextchar();
        if (c == '+' || c == '-')
        {
            append2_string(p, c);
            c = nextchar();
        }
        
        for (; is_digit(c, 10); c = nextchar())
        {
            append2_string(p, c);
        }
    }

    if (c == 'f' || c == 'F' || c == 'l' || c == 'L')
    {
        append2_string(p, c);
    }
    return true;
}

static bool
make_string_literal(Token *tk)
{
    String *str;
    int c, d;

    str = make_string();
    tk->kind = TK_STRING;
    tk->row = line;
    tk->col = column;
    tk->str = str;

    for (;;)
    {
        c = nextchar();
        if (c == '\n') return false;
        if (c == '"')
        {
            append2_string(str, '\0');
            return true;
        }
        append2_string(str, c);
        if (c == '\\')
        {
            d = nextchar();
            if (is_simple_escape(d))
            {
                append2_string(str, d);
            }
            else
            {
                print_error(tk->row, tk->col,
                        "Invalid escape character \"%c\".", (char)d);
            }
        }
    }
}

static bool
make_punctuator(Token *tk, int c)
{
    tk->id = P_INVALID;
    tk->row = line;
    tk->col = column;
    tk->str = NULL;
    switch (c)
    {
    case '[': tk->id = P_SQR_BRCK_L; break;
    case ']': tk->id = P_SQR_BRCK_R; break;
    case '(': tk->id = P_PAREN_L; break;
    case ')': tk->id = P_PAREN_R; break;
    case '{': tk->id = P_CRL_BRCK_L; break;
    case '}': tk->id = P_CRL_BRCK_R; break;
    case '.':
        if (estimate('.'))
        {
            if (estimate('.'))
            {
                tk->col--;
                tk->id = P_TLEAD;
            }
            else
            {
                // error
            }
        }
        else
        {
            tk->id = P_DOT;
        }
        break;
    case ',': tk->id = P_COMMA; break;
    case '-':
        c = nextchar();
        if (c == '>')      tk->id = P_ARRW;
        else if (c == '-') tk->id = P_MINS_MINS;
        else if (c == '=') tk->id = P_ASGN_MINS;
        else
        {
            tk->id = P_MINS;
            pushback(c);
        }
        break;
    case '+':
        c = nextchar();
        if (c == '+')      tk->id = P_PLUS_PLUS;
        else if (c == '=') tk->id = P_ASGN_PLUS;
        else
        {
            tk->id = P_PLUS;
            pushback(c);
        }
        break;
    case '&':
        c = nextchar();
        if (c == '&')      tk->id = P_AMPD_AMPD;
        else if (c == '=') tk->id = P_ASGN_AMPD;
        else
        {
            tk->id = P_AMPD;
            pushback(c);
        }
        break;
    case '*': tk->id = estimate('=') ? P_ASGN_MULT : P_MULT; break;
    case '~': tk->id = P_TILDE; break;
    case '!': tk->id = estimate('=') ? P_NEQ       : P_EXCM; break;
    case '/': tk->id = estimate('=') ? P_ASGN_DIV  : P_DIV;  break;
    case '%': tk->id = estimate('=') ? P_ASGN_MOD  : P_MOD;  break;
    case '<':
        c = nextchar();
        if (c == '<')      tk->id = estimate('=') ? P_ASGN_SHIFT_L : P_SHIFT_L;
        else if (c == '=') tk->id = P_LESS_EQ;
        else
        {
            tk->id = P_LESS;
            pushback(c);
        }
        break;
    case '>':
        c = nextchar();
        if (c == '>')      tk->id = estimate('=') ? P_ASGN_SHIFT_R : P_SHIFT_R;
        else if (c == '=') tk->id = P_GRT_EQ;
        else
        {
            tk->id = P_GRT;
            pushback(c);
        }
        break;
    case '=': tk->id = estimate('=') ? P_EQ         : P_ASGN;  break;
    case '^': tk->id = estimate('=') ? P_ASGN_CARET : P_CARET; break;
    case '|':
        c = nextchar();
        if (c == '|')      tk->id = P_VBAR_VBAR;
        else if (c == '=') tk->id = P_ASGN_VBAR;
        else               tk->id = P_VBAR;
        break;
    case '?': tk->id = P_QMARK;  break;
    case ':': tk->id = P_COLON;  break;
    case ';': tk->id = P_SCOLON; break;
    }

    if (tk->id == P_INVALID)
    {
        // error
        return false;
    }
    else
    {
        tk->kind = TK_PUNCTUATOR;
        return true;
    }
}

static bool
estimate(int x)
{
    int c = nextchar();
    if (x == c) return true; else pushback(c);
    return false;
}

static bool
is_simple_escape(int c)
{
    switch (c)
    {
    case '\'': case '"': case '?':  case '\\':
    case 'a':  case 'b': case 'f':  case 'n':
    case 'r':  case 't': case 'v':
        return true;
    default:
        return false;
    }
}

static bool
is_nondigit(int c) { return isalpha(c) || c == '_'; }

static bool
is_digit(int c, int base)
{
    switch (base)
    {
    case 8:  return isdigit(c) && c != '8' && c != '9';
    case 10: return isdigit(c);
    case 16: return isdigit(c) ||
            c == 'a' || c == 'b' || c == 'c' || c == 'd' || c == 'e' || c == 'f' ||
            c == 'A' || c == 'B' || c == 'C' || c == 'D' || c == 'E' || c == 'F';
    }
    return false;
}

static bool
is_space(int c) { return c == ' ' || c == '\t' || c == '\v'; }

static bool
is_return(int c) { return c == '\n' || c == '\r'; }

static void
set_keyword(Token *tk)
{
    int i;
    for (i = 0; keywords[i] != 0; i++)
    {
        if (cmp2_string(tk->str, keywords[i]) == 0)
        {
            tk->kind = TK_KEYWORD;
            return;
        }
    }
}

static void
skip()
{
    int c;
    for (;;)
    {
        c = nextchar();
        if (is_space(c) || is_return(c)) continue;
        if (c == '/')
        {
            if (estimate('*'))
            {
                for (;;)
                {
                    if ((c = nextchar()) == '*')
                    {
                        if (estimate('/')) break;
                    }
                }
            }
            else if (estimate('/'))
            {
                for (; !is_return(c); c = nextchar());
            }
            else
            {
                pushback(c);
                return;
            }
        }
        else
        {
            pushback(c);
            return;
        }
    }
}

static void
pushback(int c)
{
    if (sp >= STACK_SIZE)
    {
        fprintf(stderr, "Error: %d:%d The lexical analyzer stack was overflow.\n",
                line, column);
        exit(EXIT_FAILURE);
    }
    char_stack[sp++] = c;
}

static int
nextchar()
{
    int c;

    if (sp > 0) return char_stack[--sp];
    
    c = (file == NULL) ? EOF : fgetc(file);
    if (c == EOF)
    {
        close_file();
    }
    else if (c == '\n')
    {
        line++;
        column = 0;
    }
    else
    {
        column++;
    }
    return c;
}

#ifdef TEST_LEX
int
main(int argc, char *argv[])
{
    Token *token;

    if (argc != 2) exit(EXIT_FAILURE);

    open_file(argv[1]);
    for (;;) 
    {
        token = next_token();
        if (token->kind == TK_ENDFILE) break;
        print_token(token);
        free_token(&token);
    }
    return EXIT_SUCCESS;
}
#endif

