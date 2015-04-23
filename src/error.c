#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "tilde.h"

void
print_error(int row, int col, const char *f, ...)
{
    va_list list;
    fprintf(stderr, "Error:%d:%d: ", row, col);
    vfprintf(stderr, f, list);
    fprintf(stderr, "\n");
}

void
exit_error(int row, int col, const char *f, ...)
{
    va_list list;
    print_error(row, col, f, list);
    exit(EXIT_FAILURE);
}

void
malloc_error()
{
    fprintf(stderr, "Error: Not enough memory.\n");
    exit(EXIT_FAILURE);
}

