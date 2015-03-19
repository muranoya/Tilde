#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "tilde.h"

#define IDENT_MAXSIZE (256)
#define STACK_SIZE (8)

#define TEST_LEX

static FILE *file = NULL;
static int autoclose;
static int line;
static int column;

static int stack[STACK_SIZE];
static int sp;

static char *keywords[] = {
    "enum",   "unsigned", "break",  "return",  "void",     "case",
    "float",  "short",    "char",   "for",     "signed",   "while",
    "const",  "goto",     "sizeof", "bool",    "continue", "if",
    "static", "default",  "inline", "struct",  "do",       "int",
    "switch", "double",   "long",   "typedef", "else",     "union",
    "extern",
    0,
};

/* prototype */
static char *punctuator_tostring(enum PnctID p);

static int  make_endfile(struct Token *tk);
static int  make_ident(struct Token *tk, int c);
static int  make_digit(struct Token *tk, int c);
static int  make_digit_base(struct Token *tk, int c, int base, char *p);
static int  make_float_base(int c, int base, char *p);
static int  make_string_literal(struct Token *tk);
static int  make_punctuator(struct Token *tk, int c);

static int  estimate(int x);
static int  is_simple_escape();
static int  is_nondigit(int c);
static int  is_digit(int c, int base);
static int  is_space(int c);
static int  is_return(int c);

static void set_keyword(struct Token *tk);
static void skip();
static void pushback(int c);
static int  nextchar();

int
openfile(const char *f, int ac)
{
    file = fopen(f, "r");
    autoclose = ac;
    if (file == NULL)
    {
        perror("fopen");
        return 0;
    }
    line = 1;
    column = 0;
    sp = 0;
    return 1;
}

void
closefile()
{
    if (fclose(file) != 0) perror("fclose");
    file = NULL;
}

void
print_token(const struct Token *tk)
{
    char *p;
    switch (tk->kind)
    {
    case IDENT:      p = "IDENT";      break;
    case KEYWORD:    p = "KEYWORD";    break;
    case DECIMAL:    p = "DECIMAL";    break;
    case OCTAL:      p = "OCTAL";      break;
    case HEXA:       p = "HEXA";       break;
    case FLOAT:      p = "FLOAT";      break;
    case HEXAFLOAT:  p = "HEXAFLOAT";  break;
    case STRING:     p = "STRING";     break;
    case CHAR:       p = "CHAR";       break;
    case PUNCTUATOR: p = "PUNCTUATOR"; break;
    case ENDFILE:    p = "ENDFILE";    break;
    default:         p = "UNKNOWN";    break;
    }
    printf("Type:%s (%d:%d)%s\n", p, tk->row, tk->col,
            tk->kind == PUNCTUATOR ? punctuator_tostring(tk->id) : tk->val);
}

static char *
punctuator_tostring(enum PnctID p)
{
    switch (p)
    {
    case PSQR_BRCK_L:   return "[";
    case PSQR_BRCK_R:   return "]";
    case PPAREN_L:      return "(";
    case PPAREN_R:      return ")";
    case PCRL_BRCK_L:   return "{";
    case PCRL_BRCK_R:   return "}";
    case PDOT:          return ".";
    case PARRW:         return "->";
    case PPLUS_PLUS:    return "++";
    case PMINS_MINS:    return "--";
    case PAMPD:         return "&";
    case PMULT:         return "*";
    case PPLUS:         return "+";
    case PMINS:         return "-";
    case PTILDE:        return "~";
    case PEXCM:         return "!";
    case PDIV:          return "/";
    case PMOD:          return "%";
    case PSHIFT_L:      return "<<";
    case PSHIFT_R:      return ">>";
    case PLESS:         return "<";
    case PGRT:          return ">";
    case PLESS_EQ:      return "<=";
    case PGRT_EQ:       return ">=";
    case PEQ:           return "==";
    case PNEQ:          return "!=";
    case PCARET:        return "^";
    case PVBAR:         return "|";
    case PAMPD_AMPD:    return "&&";
    case PVBAR_VBAR:    return "||";
    case PQMARK:        return "?";
    case PCOLON:        return ":";
    case PSCOLON:       return ";";
    case PTLEAD:        return "...";
    case PASGN:         return "=";
    case PASGN_MULT:    return "*=";
    case PASGN_DIV:     return "/=";
    case PASGN_MOD:     return "%=";
    case PASGN_PLUS:    return "+=";
    case PASGN_MINS:    return "-=";
    case PASGN_SHIFT_L: return "<<=";
    case PASGN_SHIFT_R: return ">>=";
    case PASGN_AMPD:    return "&=";
    case PASGN_CARET:   return "^=";
    case PASGN_VBAR:    return "|=";
    default:            return "UNKNOWN";
    }
}

int
nexttoken(struct Token *token)
{
    int c;
    skip();
    c = nextchar();

    token->kind = INVALID;
    token->id = PINVALID;
    token->row = line;
    token->col = column;
    token->val = NULL;


    if (is_nondigit(c))
    {
        return make_ident(token, c);
    }
    else if (is_digit(c, 10))
    {
        return make_digit(token, c);
    }
    else if (c == '"')
    {
        return make_string_literal(token);
    }
    else if (c == '\'')
    {
        char *p = (char*)malloc(sizeof(char)*8);
        token->kind = CHAR;
        token->row = line;
        token->col = column-1;
        token->val = p;
        c = nextchar();
        if (c == '\\')
        {
            if (is_simple_escape())
            {
                *p++ = '\\';
                *p++ = nextchar();
            }
            else
            {
                //error
            }
        }
        else
        {
            *p++ = c;
        }
        return estimate('\'');
    }
    else if (c == '.')
    {
        c = nextchar();
        if (is_digit(c, 10))
        {
            char *p = (char*)malloc(sizeof(char)*IDENT_MAXSIZE);
            pushback(c);
            token->kind = FLOAT;
            token->row = line;
            token->col = column-1;
            token->val = p;
            return make_float_base('.', 10, p);
        }
        else
        {
            pushback(c);
        }
    }
    else if (c == EOF)
    {
        return make_endfile(token);
    }

    if (make_punctuator(token, c))
    {
        return 1;
    }
    else
    {
        pushback(c);
    }
    return 0;
}

static int
make_endfile(struct Token *tk)
{
    tk->kind = ENDFILE;
    tk->row = line;
    tk->col = column;
    tk->val = "";
    return 0;
}

static int
make_ident(struct Token *tk, int c)
{
    char *ident = (char*)malloc(sizeof(char) * IDENT_MAXSIZE);
    int len = 0;
    tk->kind = IDENT;
    tk->row = line;
    tk->col = column;
    tk->val = ident;
    for (; is_nondigit(c) || is_digit(c, 10); c = nextchar())
    {
        ident[len++] = c;
    }
    ident[len] = 0;
    pushback(c);
    set_keyword(tk);
    return 1;
}

static int
make_digit(struct Token *tk, int c)
{
    char *val = (char*)malloc(sizeof(char)*IDENT_MAXSIZE);
    int ret;
    tk->row = line;
    tk->col = column;
    tk->val = val;
    if (c == '0')
    {
        c = nextchar();
        if (c == 'x' || c == 'X')
        {
            tk->kind = HEXA;
            ret = make_digit_base(tk, nextchar(), 16, val);
        }
        else
        {
            tk->kind = OCTAL;
            ret = make_digit_base(tk, c, 8, val);
        }
    }
    else
    {
        tk->kind = DECIMAL;
        ret = make_digit_base(tk, c, 10, val);
    }
    return ret;
}

static int
make_digit_base(struct Token *tk, int c, int base, char *p)
{
    int len = 0;
    for (; is_digit(c, base); c = nextchar())
    {
        p[len++] = c;
    }

    if (c == '.' || c == 'e' || c == 'E' || c == 'p' || c == 'P')
    {
        tk->kind = (base == 16) ? HEXAFLOAT : FLOAT;
        return make_float_base(c, (base == 16) ? 16 : 10, p+len);
    }

    if (c == 'u' || c == 'U' || c == 'l' || c == 'L')
    {
        int d = nextchar();
        p[len++] = c;
        if (((c == 'u' || c == 'U') && (d == 'l' || d == 'L')) ||
            ((c == 'l' || c == 'L') && (d == 'u' || d == 'U')))
        {
            p[len++] = d;
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
    p[len] = 0;
    return 1;
}

static int
make_float_base(int c, int base, char *p)
{
    int len = 0;

    if (!(base == 10 && (c == '.' || c == 'e' || c == 'E')) &&
        !(base == 16 && (c == '.' || c == 'p' || c == 'P')))
    {
        //error
        return 0;
    }

    if (c == '.')
    {
        p[len++] = c;
        c = nextchar();
        if (is_digit(c, base))
        {
            for (; is_digit(c, base); c = nextchar())
            {
                p[len++] = c;
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
        p[len++] = c;
        c = nextchar();
        if (c == '+' || c == '-')
        {
            p[len++] = c;
            c = nextchar();
        }
        
        for (; is_digit(c, 10); c = nextchar())
        {
            p[len++] = c;
        }
    }

    if (c == 'f' || c == 'F' || c == 'l' || c == 'L')
    {
        p[len++] = c;
    }
    p[len] = 0;
    return 1;
}

static int
make_string_literal(struct Token *tk)
{
    char *str = (char*)malloc(sizeof(char)*IDENT_MAXSIZE);
    int size = IDENT_MAXSIZE;
    int c;
    int len = 0;
    tk->row = line;
    tk->col = column;
    tk->val = str;
    tk->kind = STRING;
    for (;;)
    {
        c = nextchar();
        if (c == '\n')
        {
            return 0;
        }
        else if (c == '"')
        {
            str[len] = 0;
            return 1;
        }
        else
        {
            if (len+2 >= size)
            {
                str = (char*)realloc(str, sizeof(char)*(size+IDENT_MAXSIZE));
            }

            if (c == '\\' && is_simple_escape())
            {
                str[len++] = '\\';
                str[len++] = nextchar();
            }
            else
            {
                str[len++] = c;
            }
        }
    }
}

static int
make_punctuator(struct Token *tk, int c)
{
    tk->id = PINVALID;
    tk->row = line;
    tk->col = column;
    tk->val = NULL;
    switch (c)
    {
    case '[':
        tk->id = PSQR_BRCK_L; break;
    case ']':
        tk->id = PSQR_BRCK_R; break;
    case '(':
        tk->id = PPAREN_L; break;
    case ')':
        tk->id = PPAREN_R; break;
    case '{':
        tk->id = PCRL_BRCK_L; break;
    case '}':
        tk->id = PCRL_BRCK_R; break;
    case '.':
        if (estimate('.'))
        {
            if (estimate('.'))
            {
                tk->id = PTLEAD;
            }
            else
            {
                // error
            }
        }
        else
        {
            tk->id = PDOT;
        }
        break;
    case '-':
        c = nextchar();
        if (c == '>')      tk->id = PARRW;
        else if (c == '-') tk->id = PMINS_MINS;
        else if (c == '=') tk->id = PASGN_MINS;
        else
        {
            tk->id = PMINS;
            pushback(c);
        }
        break;
    case '+':
        c = nextchar();
        if (c == '+')      tk->id = PPLUS_PLUS;
        else if (c == '=') tk->id = PASGN_PLUS;
        else
        {
            tk->id = PPLUS;
            pushback(c);
        }
        break;
    case '&':
        c = nextchar();
        if (c == '&')      tk->id = PAMPD_AMPD;
        else if (c == '=') tk->id = PASGN_AMPD;
        else
        {
            tk->id = PAMPD;
            pushback(c);
        }
        break;
    case '*':
        tk->id = estimate('=') ? PASGN_MULT : PMULT;
        break;
    case '~':
        tk->id = PTILDE; break;
    case '!':
        tk->id = estimate('=') ? PNEQ : PEXCM;
        break;
    case '/':
        tk->id = estimate('=') ? PASGN_DIV : PDIV;
        break;
    case '%':
        tk->id = estimate('=') ? PASGN_MOD : PMOD;
        break;
    case '<':
        c = nextchar();
        if (c == '<')
        {
            tk->id = estimate('=') ? PASGN_SHIFT_L : PSHIFT_L;
        }
        else if (c == '=')
        {
            tk->id = PLESS_EQ;
        }
        else
        {
            tk->id = PLESS;
            pushback(c);
        }
        break;
    case '>':
        c = nextchar();
        if (c == '>')
        {
            tk->id = estimate('=') ? PASGN_SHIFT_R : PSHIFT_R;
        }
        else if (c == '=')
        {
            tk->id = PGRT_EQ;
        }
        else
        {
            tk->id = PGRT;
            pushback(c);
        }
        break;
    case '=':
        tk->id = estimate('=') ? PEQ : PASGN;
        break;
    case '^':
        tk->id = estimate('=') ? PASGN_CARET : PCARET;
        break;
    case '|':
        c = nextchar();
        if (c == '|')
        {
            tk->id = PVBAR_VBAR;
        }
        else if (c == '=')
        {
            tk->id = PASGN_VBAR;
        }
        else
        {
            tk->id = PVBAR;
        }
        break;
    case '?':
        tk->id = PQMARK;
        break;
    case ':':
        tk->id = PCOLON;
        break;
    case ';':
        tk->id = PSCOLON;
        break;
    }

    if (tk->id != PINVALID)
    {
        tk->kind = PUNCTUATOR;
        return 1;
    }
    else
    {
        return 0;
    }
}

static int
estimate(int x)
{
    int c = nextchar();
    if (x == c) return 1; else pushback(c);
    return 0;
}

static int
is_simple_escape()
{
    int c = nextchar();
    switch (c)
    {
    case '\'': case '"': case '?':  case '\\':
    case 'a':  case 'b': case 'f':  case 'n':
    case 'r':  case 't': case 'v':
        pushback(c);
        return 1;
    default:
        pushback(c);
        return 0;
    }
}

static int
is_nondigit(int c) { return isalpha(c) || c == '_'; }

static int
is_digit(int c, int base)
{
    switch (base)
    {
    case 8:
        return isdigit(c) && c != '8' && c != '9';
        break;
    case 10:
        return isdigit(c);
        break;
    case 16:
        return isdigit(c) ||
            c == 'a' || c == 'b' || c == 'c' || c == 'd' || c == 'e' || c == 'f' ||
            c == 'A' || c == 'B' || c == 'C' || c == 'D' || c == 'E' || c == 'F';
        break;
    default:
        ;
    }
    return 0;
}

static int
is_space(int c) { return c == ' ' || c == '\t' || c == '\v'; }

static int
is_return(int c) { return c == '\n' || c == '\r'; }

static void
set_keyword(struct Token *tk)
{
    int i;
    for (i = 0; keywords[i] != 0; i++)
    {
        if (strcmp(keywords[i], tk->val) == 0)
        {
            tk->kind = KEYWORD;
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
    if (sp > 0)
    {
        return stack[--sp];
    }
    
    c = (file == NULL) ? EOF : fgetc(file);
    if (c == EOF)
    {
        if (autoclose)
        {
            closefile();
        }
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
    struct Token token;

    if (argc != 2)
    {
        exit(EXIT_FAILURE);
    }

    openfile(argv[1], 1);
    for (; nexttoken(&token) != 0;)
    {
        print_token(&token);
    }
}
#endif

