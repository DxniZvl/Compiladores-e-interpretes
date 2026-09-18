#include <stdio.h>

int yylex(void);
extern int lexical_errors;

void show_token(const char *kind, const char *text)
{
    printf("%-12s %s\n", kind, text);
}

int main(void)
{
    yylex();
    return lexical_errors ? 1 : 0;
}
