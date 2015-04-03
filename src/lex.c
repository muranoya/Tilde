#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "tilde.h"

#define STACK_SIZE (8)

static FILE *file = NULL;
static bool autoclose;
static int line, column;

static int stack[STACK_SIZE], sp;

static char *keywords[] =
{
    "enum",   "unsigned", "break",  "return",  "void",     "case",
    "float",  "short",    "char",   "for",     "signed",   "while",
    "const",  "goto",     "sizeof", "bool",    "continue", "if",
    "static", "default",  "inline", "struct",  "do",       "int",
    "switch", "double",   "long",   "typedef", "else",     "union",
    "extern",
    0,
};

/* prototype */
static bool make_ident(Token *tk, int c);
static bool make_digit(Token *tk, int c);
static bool make_digit_base(Token *tk, int c, int base, String *p);
static bool make_float_base(int c, int base, String *p);
static bool make_string_literal(Token *tk);
static bool make_punctuator(Token *tk, int c);

static bool estimate(int x);
static bool is_simple_escape();
static bool is_nondigit(int c);
static bool is_digit(int c, int base);
static bool is_space(int c);
static bool is_return(int c);

static void set_keyword(Token *tk);
static void skip();
static void pushback(int c);
static int  nextchar();

bool
open_file(const char *f, bool ac)
{
    file = fopen(f, "r");
    autoclose = ac;
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
    if (fclose(file) != 0) perror("fclose");
    file = NULL;
}

void
print_token(const Token *tk)
{
    char *p;
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
    printf("%s (%d:%d)%s\n",
            p,
            tk->row,
            tk->col,
            tk->kind == TK_PUNCTUATOR ? punctuator_to_string(tk->id)
                                      : tk->str->str);
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

void
next_token(Token *token)
{
    int c;
    skip();
    c = nextchar();

    token->kind = TK_INVALID;
    token->id = P_INVALID;
    token->row = line;
    token->col = column;
    token->str = NULL;

    if (is_nondigit(c))
    {
        make_ident(token, c);
        return;
    }
    else if (is_digit(c, 10))
    {
        make_digit(token, c);
        return;
    }
    else if (c == '"')
    {
        make_string_literal(token);
        return;
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
            if (is_simple_escape())
            {
                append2_string(p, '\\');
                append2_string(p, nextchar());
            }
            else
            {
                //error
            }
        }
        else
        {
            append2_string(p, c);
        }
        estimate('\'');
        return;
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
            return;
        }
        else
        {
            pushback(c);
        }
    }
    else if (c == EOF)
    {
        token->kind = TK_ENDFILE;
        token->row = line;
        token->col = column;
        token->str = NULL;
        return;
    }

    if (make_punctuator(token, c))
    {
        return;
    }
    else
    {
        pushback(c);
    }

    fprintf(stderr, "Error:%d:%d: Invalid token.\n", line, column);
    exit(EXIT_FAILURE);
}

static bool
make_ident(Token *tk, int c)
{
    String *ident;

    ident = make_string();
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
    String *val;
    bool ret;

    val = make_string();
    tk->kind = TK_CONSTANT;
    tk->row = line;
    tk->col = column;
    tk->str = val;
    if (c == '0')
    {
        c = nextchar();
        if (c == 'x' || c == 'X')
        {
            append3_string(val, "0x");
            ret = make_digit_base(tk, nextchar(), 16, val);
        }
        else
        {
            append2_string(val, '0');
            ret = make_digit_base(tk, c, 8, val);
        }
    }
    else
    {
        ret = make_digit_base(tk, c, 10, val);
    }
    return ret;
}

static bool
make_digit_base(Token *tk, int c, int base, String *p)
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
    int c;

    str = make_string();
    tk->kind = TK_STRING;
    tk->row = line;
    tk->col = column;
    tk->str = str;

    for (;;)
    {
        c = nextchar();
        if (c == '\n')
        {
            return false;
        }
        else if (c == '"')
        {
            append2_string(str, '\0');
            return true;
        }
        else
        {
            if (c == '\\' && is_simple_escape())
            {
                append2_string(str, '\\');
                append2_string(str, nextchar());
            }
            else
            {
                append2_string(str, c);
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
    case '[':
        tk->id = P_SQR_BRCK_L; break;
    case ']':
        tk->id = P_SQR_BRCK_R; break;
    case '(':
        tk->id = P_PAREN_L; break;
    case ')':
        tk->id = P_PAREN_R; break;
    case '{':
        tk->id = P_CRL_BRCK_L; break;
    case '}':
        tk->id = P_CRL_BRCK_R; break;
    case '.':
        if (estimate('.'))
        {
            if (estimate('.'))
            {
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
    case ',':
        tk->id = P_COMMA; break;
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
    case '*':
        tk->id = estimate('=') ? P_ASGN_MULT : P_MULT;
        break;
    case '~':
        tk->id = P_TILDE; break;
    case '!':
        tk->id = estimate('=') ? P_NEQ : P_EXCM;
        break;
    case '/':
        tk->id = estimate('=') ? P_ASGN_DIV : P_DIV;
        break;
    case '%':
        tk->id = estimate('=') ? P_ASGN_MOD : P_MOD;
        break;
    case '<':
        c = nextchar();
        if (c == '<')
        {
            tk->id = estimate('=') ? P_ASGN_SHIFT_L : P_SHIFT_L;
        }
        else if (c == '=')
        {
            tk->id = P_LESS_EQ;
        }
        else
        {
            tk->id = P_LESS;
            pushback(c);
        }
        break;
    case '>':
        c = nextchar();
        if (c == '>')
        {
            tk->id = estimate('=') ? P_ASGN_SHIFT_R : P_SHIFT_R;
        }
        else if (c == '=')
        {
            tk->id = P_GRT_EQ;
        }
        else
        {
            tk->id = P_GRT;
            pushback(c);
        }
        break;
    case '=':
        tk->id = estimate('=') ? P_EQ : P_ASGN;
        break;
    case '^':
        tk->id = estimate('=') ? P_ASGN_CARET : P_CARET;
        break;
    case '|':
        c = nextchar();
        if (c == '|')
        {
            tk->id = P_VBAR_VBAR;
        }
        else if (c == '=')
        {
            tk->id = P_ASGN_VBAR;
        }
        else
        {
            tk->id = P_VBAR;
        }
        break;
    case '?':
        tk->id = P_QMARK;
        break;
    case ':':
        tk->id = P_COLON;
        break;
    case ';':
        tk->id = P_SCOLON;
        break;
    }

    if (tk->id == P_INVALID)
    {
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
is_simple_escape()
{
    int c = nextchar();
    switch (c)
    {
    case '\'': case '"': case '?':  case '\\':
    case 'a':  case 'b': case 'f':  case 'n':
    case 'r':  case 't': case 'v':
        pushback(c);
        return true;
    default:
        pushback(c);
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
    case 8:
        return isdigit(c) && c != '8' && c != '9';
    case 10:
        return isdigit(c);
    case 16:
        return isdigit(c) ||
            c == 'a' || c == 'b' || c == 'c' || c == 'd' || c == 'e' || c == 'f' ||
            c == 'A' || c == 'B' || c == 'C' || c == 'D' || c == 'E' || c == 'F';
    default:
        //error
        ;
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
        if (strcmp(keywords[i], tk->str->str) == 0)
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
        if (is_space(c) || is_return(c))
        {
            continue;
        }
        else if (c == '/')
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
    stack[sp++] = c;
}

static int
nextchar()
{
    int c;

    if (sp > 0) return stack[--sp];
    
    c = (file == NULL) ? EOF : fgetc(file);
    if (c == EOF)
    {
        if (autoclose) close_file();
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
    Token token;

    if (argc != 2) exit(EXIT_FAILURE);

    open_file(argv[1], 1);
    for (;;) 
    {
        next_token(&token);
        if (token.kind == TK_ENDFILE) break;
        print_token(&token);
    }
    return EXIT_SUCCESS;
}
#endif

