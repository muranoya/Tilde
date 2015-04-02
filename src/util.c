#include <string.h>
#include <stdlib.h>

char *
newString(const char *str)
{
    char *s;
    s = (char*)malloc(sizeof(char)*(strlen(str)+1));
    strcpy(s, str);
    return s;
}

