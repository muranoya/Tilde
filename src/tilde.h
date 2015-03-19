#ifndef TILDE_H
#define TILDE_H

enum TokenKind
{
    IDENT,
    KEYWORD,
    DECIMAL, OCTAL, HEXA,
    FLOAT, HEXAFLOAT,
    STRING,
    CHAR,
    PUNCTUATOR,
    ENDFILE,
    INVALID,
};

enum PnctID
{
    PSQR_BRCK_L, PSQR_BRCK_R,
    PPAREN_L,    PPAREN_R,
    PCRL_BRCK_L, PCRL_BRCK_R,
    PDOT, PARRW,
    
    PPLUS_PLUS,
    PMINS_MINS,  PAMPD,
    PMULT, PPLUS, PMINS,
    PTILDE, PEXCM,

    PDIV, PMOD, PSHIFT_L, PSHIFT_R,
    PLESS, PGRT, PLESS_EQ, PGRT_EQ,
    PEQ, PNEQ, PCARET, PVBAR,
    PAMPD_AMPD, PVBAR_VBAR,
    PQMARK, PCOLON, PSCOLON, PTLEAD,

    PASGN, PASGN_MULT, PASGN_DIV,
    PASGN_MOD, PASGN_PLUS, PASGN_MINS,
    PASGN_SHIFT_L, PASGN_SHIFT_R,
    PASGN_AMPD, PASGN_CARET, PASGN_VBAR,

    PINVALID,
};

struct Token
{
    enum TokenKind kind;
    enum PnctID id;
    char *val;
    int row;
    int col;
};

// lex.c
int  openfile(const char *f, int ac);
void closefile();
void print_token(const struct Token *tk);
char *print_punctuator(enum PnctID p);
int  nexttoken(struct Token *token);
#endif
