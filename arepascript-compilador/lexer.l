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

"//"[^\n]*              ; /* Comentario de una linea */
[ \t\r\n]+              ; /* Espacios en blanco */

"vaina"                 { return TOKEN_VAINA; }
"pichirre"              { return TOKEN_PICHIRRE; }
"echale"                { return TOKEN_ECHALE; }

"ta_completo"|"quebrao"|"cuento"|"letra"|"si_o_no" {
                          yylval.str_val = string_copy(yytext);
                          return TOKEN_TYPE;
                        }

"chevere"               { yylval.bool_value = 1; return TOKEN_BOOL_LITERAL; }
"nada_que_ver"          { yylval.bool_value = 0; return TOKEN_BOOL_LITERAL; }

":"                     { return ':'; }
"="                     { return '='; }
";"                     { return ';'; }
"+"                     { return '+'; }
"-"                     { return '-'; }
"*"                     { return '*'; }
"/"                     { return '/'; }
"("                     { return '('; }
")"                     { return ')'; }

\"([^\\\"]|\\.)*\"     {
                         /* Quitar comillas */
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

.                      { fprintf(stderr, "Caracter inesperado: %s\n", yytext); }

%%
