%{
#include "ast.h"
#include "parser.tab.h"
#include "string_utils.h"
#include <string.h>
#include <stdlib.h>
#include "lexer.h"
%}

%option noyywrap

%%

[ \t\r\n]+             ; /* Skip whitespace */

"var"                  { return TOKEN_VAR; }
"const"                { return TOKEN_CONST; }
"print"                { return TOKEN_PRINT; }
"printv"               { return TOKEN_PRINTV; }
"int"|"float"|"bool"|"char"|"str"|"string" {
                          yylval.str_val = string_copy(yytext);
                          return TOKEN_TYPE;
                        }

":"                    { return ':'; }
"="                    { return '='; }
";"                    { return ';'; }
"+"                    { return '+'; }
"-"                    { return '-'; }
"*"                    { return '*'; }
"/"                    { return '/'; }
"("                    { return '('; }
")"                    { return ')'; }


"true"        { yylval.bool_value = 1; return TOKEN_BOOL_LITERAL; }
"false"       { yylval.bool_value = 0; return TOKEN_BOOL_LITERAL; }

\"([^\\\"]|\\.)*\"     {
                         /* Strip quotes */
                         yytext[yyleng - 1] = '\0';
                          yylval.str_val = string_copy(yytext + 1);
                         return TOKEN_STRING_LITERAL;
                       }
\'([^'\\\n]|\\.)\' {
    yylval.char_val = parse_char_escape(yytext);
    return TOKEN_CHAR_LITERAL;
}

[0-9]+\.[0-9]+         { yylval.float_value = (float)atof(yytext); return TOKEN_FLOAT_LITERAL; }
[0-9]+                 { yylval.int_val = atoi(yytext); return TOKEN_INT_LITERAL; }
[a-zA-Z_][a-zA-Z0-9_]* { yylval.str_val = string_copy(yytext); return TOKEN_IDENTIFIER; }

.                      { fprintf(stderr, "Unexpected char: %s\n", yytext); }

%%
