#ifndef TILDE_H
#define TILDE_H

enum TokenKind
{
    TK_KEYWORD,
    TK_IDENT,
    TK_CONSTANT,
    TK_STRING,
    TK_PUNCTUATOR,
    TK_ENDFILE,
    TK_INVALID,
};

enum ConstKind
{
    CK_CHAR,
    CK_DECIMAL,
    CK_OCTAL,
    CK_HEXA,
    CK_FLOAT,
    CK_HEXAFLOAT,
    CK_INVALID,
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
    PQMARK, PCOLON, PSCOLON, PTLEAD, PCOMMA,

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
    enum ConstKind ckind;
    char *str;
    int row;
    int col;
};

// lex.c
int  openFile(const char *f, int ac);
void closeFile();
void printToken(const struct Token *tk);
char *punctuatorToString(enum PnctID p);
int  nextToken(struct Token *token);
struct Token *nextToken2();
#endif
