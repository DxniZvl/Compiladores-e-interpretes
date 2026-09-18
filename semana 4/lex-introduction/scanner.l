%option noyywrap
%{
#include <stdio.h>

void show_token(const char *kind, const char *text);
int lexical_errors = 0;
%}

%%
"let"                       { show_token("LET", yytext); }
"print"                     { show_token("PRINT", yytext); }
"if"                        { show_token("IF", yytext); }
[0-9]+                      { show_token("NUMBER", yytext); }
[a-zA-Z_][a-zA-Z0-9_]*       { show_token("IDENTIFIER", yytext); }
"="                         { show_token("ASSIGN", yytext); }
"+"                         { show_token("PLUS", yytext); }
";"                         { show_token("SEMICOLON", yytext); }
[ \t\r\n]+                  { /* Ignore whitespace. */ }
.                           { show_token("ERROR", yytext); lexical_errors++; }
%%
