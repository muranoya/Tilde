#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "tilde.h"

#define IDENT_MAXSIZE (256)
#define STACK_SIZE (8)
#define TEST

static FILE *file = NULL;
static int line;
static int column;

static int stack[STACK_SIZE];
static int sp;

/* prototype */
static int  make_ident(struct Token *tk, int c);
static int  make_digit(struct Token *tk, int c);
static int  make_digit_base(struct Token *tk, int c, int base, char *p);
static int  make_float_base(int c, int base, char *p);
static int  make_string_literal(struct Token *tk);

static int  is_simple_escape();
static int  is_nondigit(int c);
static int  is_digit(int c, int base);
static int  is_nonzerodigit(int c, int base);
static int  is_space(int c);
static int  is_return(int c);

static void skip();
static void pushback(int c);
static int  nextchar();

int
openfile(char *f)
{
    file = fopen(f, "r");
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
    if (fclose(file) != 0)
    {
        perror("fclose");
    }
}

int
nexttoken(struct Token *token)
{
    int c;
    skip();
    c = nextchar();

    if (is_nondigit(c))
    {
        return make_ident(token, c);
    }
    else if (is_digit(c, 10))
    {
        return make_digit(token, c);
    }
    else if (c == '.')
    {
        c = nextchar();
        if (is_digit(c, 10))
        {
            pushback(c);
            char *p = (char*)malloc(sizeof(char)*IDENT_MAXSIZE);
            token->kind = FLOAT;
            token->row = line;
            token->col = column;
            token->val = p;
            return make_float_base('.', 10, p);
        }
    }
    else if (c == '"')
    {
        return make_string_literal(token);
    }
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
        return make_float_base(c, base == 16 ? 16 : 10, p+len);
    }

    if (c == 'u' || c == 'U')
    {
        p[len++] = c;
        c = nextchar();
        if (c == 'l' || c == 'L')
        {
            p[len++] = c;
        }
        else
        {
            pushback(c);
        }
    }
    else if (c == 'l' || c == 'L')
    {
        p[len++] = c;
        c = nextchar();
        if (c == 'u' || c == 'U')
        {
            p[len++] = c;
        }
        else
        {
            pushback(c);
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
is_simple_escape()
{
    int c = nextchar();
    switch (c)
    {
    case '\'': case '"':
    case '?':  case '\\':
    case 'a':  case 'b':
    case 'f':  case 'n':
    case 'r':  case 't':
    case 'v':
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
is_nonzerodigit(int c, int base) { return is_digit(c, base) && c != '0'; }

static int
is_space(int c) { return c == ' ' || c == '\t'; }

static int
is_return(int c) { return c == '\n' || c == '\r'; }

static void
skip()
{
    int c, d;
    for (;;)
    {
        c = nextchar();
        if (is_space(c) || is_return(c))
        {
            continue;
        }
        else if (c == '/')
        {
            if ((d = nextchar()) == '*')
            {
                for (;;)
                {
                    if ((c = nextchar()) == '*')
                    {
                        if ((d = nextchar()) == '/')
                        {
                            break;
                        }
                        else
                        {
                            pushback(d);
                        }
                    }
                }
            }
            else if (d == '/')
            {
                for (; !is_return(c); c = nextchar());
            }
            else
            {
                pushback(d);
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
    
    c = fgetc(file);
    if (c == '\n')
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

#ifdef TEST
int
main(int argc, char *argv[])
{
    struct Token token;

    if (argc != 2)
    {
        exit(EXIT_FAILURE);
    }

    openfile(argv[1]);
    for (; nexttoken(&token) != 0;)
    {
        printf("%d:%d:%s\n", token.row, token.col, token.val);
    }
    closefile();
}
#endif

