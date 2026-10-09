%{
#include "ast.h"
#include "symbol_table.h"
#include <stdio.h>
#include <stdlib.h>

extern int yylex();
void yyerror(const char *s);

ASTNode *ast_root = NULL;
SymbolTable symbol_table;

%}

%union {
    int int_val;
    char *str_val;
    char char_val;
    float float_value;
    int bool_value;
    struct ASTNode *node;
}

%token TOKEN_VAINA TOKEN_PICHIRRE TOKEN_ECHALE
%token <str_val> TOKEN_TYPE
%token <str_val> TOKEN_IDENTIFIER
%token <str_val> TOKEN_STRING_LITERAL
%token <int_val> TOKEN_INT_LITERAL
%token <float_value> TOKEN_FLOAT_LITERAL
%token <bool_value> TOKEN_BOOL_LITERAL
%token <char_val> TOKEN_CHAR_LITERAL

%left '+' '-'
%left '*' '/'

/* Este archivo solo construye el AST (analisis sintactico puro).
   Ninguna regla aqui consulta la tabla de simbolos ni valida tipos:
   eso es trabajo del analizador semantico (semantic_analyzer.c), que
   corre despues sobre el arbol ya construido. */
%type <node> program statement_list statement var_declaration
%type <node> assignment_statement echale_statement expression

%%

program:
    statement_list {
        ast_root = $1;
    }
    ;

statement_list:
    statement {
        $$ = $1;
    }
    | statement_list statement {
        ASTNode *cur = $1;
        while (cur->next != NULL) {
            cur = cur->next;
        }
        cur->next = $2;
        $$ = $1;
    }
    ;

statement:
    var_declaration { $$ = $1; }
    | assignment_statement { $$ = $1; }
    | echale_statement { $$ = $1; }
    ;

var_declaration:
    TOKEN_VAINA TOKEN_IDENTIFIER ':' TOKEN_TYPE ';' {
        DataType *type = parse_string_data_type($4);
        $$ = create_var_decl(type, $2, NULL);
        free($2);
        free($4);
    }
    | TOKEN_VAINA TOKEN_IDENTIFIER ':' TOKEN_TYPE '=' expression ';' {
        DataType *type = parse_string_data_type($4);
        $$ = create_var_decl(type, $2, $6);
        free($2);
        free($4);
    }
    | TOKEN_VAINA TOKEN_IDENTIFIER '=' expression ';' {
        /* tipo automatico: se infiere en el analizador semantico */
        $$ = create_var_decl(NULL, $2, $4);
        free($2);
    }
    | TOKEN_PICHIRRE TOKEN_IDENTIFIER ':' TOKEN_TYPE '=' expression ';' {
        DataType *type = parse_string_data_type($4);
        $$ = create_const_decl(type, $2, $6);
        free($2);
        free($4);
    }
    | TOKEN_PICHIRRE TOKEN_IDENTIFIER '=' expression ';' {
        $$ = create_const_decl(NULL, $2, $4);
        free($2);
    }
    ;

assignment_statement:
    TOKEN_IDENTIFIER '=' expression ';' {
        $$ = create_assign_node($1, $3);
        free($1);
    }
    ;

echale_statement:
    TOKEN_ECHALE expression ';' {
        $$ = create_print_node($2);
    }
    ;

expression:
    TOKEN_INT_LITERAL { $$ = create_int_node($1); }
    | TOKEN_FLOAT_LITERAL { $$ = create_float_node($1); }
    | TOKEN_STRING_LITERAL {
        $$ = create_str_node($1);
        free($1);
    }
    | TOKEN_BOOL_LITERAL { $$ = create_bool_node($1); }
    | TOKEN_CHAR_LITERAL { $$ = create_char_node($1); }
    | TOKEN_IDENTIFIER {
        $$ = create_identifier_node($1, TYPE_UNKNOWN);
        free($1);
    }
    | '(' expression ')' { $$ = $2; }
    | expression '+' expression { $$ = create_binary_node(OP_ADD, $1, $3); }
    | expression '-' expression { $$ = create_binary_node(OP_SUB, $1, $3); }
    | expression '*' expression { $$ = create_binary_node(OP_MUL, $1, $3); }
    | expression '/' expression { $$ = create_binary_node(OP_DIV, $1, $3); }
    ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "Error de sintaxis: %s\n", s);
}
