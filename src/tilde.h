enum TokenKind
{
    IDENT,
    DECIMAL,
    OCTAL,
    HEXA,
    FLOAT,
    HEXAFLOAT,
    STRING,
};

struct Token
{
    enum TokenKind kind;
    char *val;
    int row;
    int col;
};

int openfile(char *f);
void closefile();
int nexttoken(struct Token *token);
