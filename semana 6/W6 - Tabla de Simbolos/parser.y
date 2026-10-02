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

%token TOKEN_VAR TOKEN_CONST TOKEN_PRINT TOKEN_PRINTV
%token <str_val> TOKEN_TYPE
%token <str_val> TOKEN_IDENTIFIER
%token <str_val> TOKEN_STRING_LITERAL
%token <int_val> TOKEN_INT_LITERAL
%token <float_value> TOKEN_FLOAT_LITERAL
%token <bool_value> TOKEN_BOOL_LITERAL
%token <char_val> TOKEN_CHAR_LITERAL

%left '+' '-'
%left '*' '/'

//any of these non-terminal rules produce a result
%type <node> program statement_list statement var_declaration print_statement expression

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
        /* Append to linked list */
        ASTNode *cur = $1;
        while (cur->next != NULL) {
            cur = cur->next;
        }
        cur->next = $2;
        $$ = $1;
    }
    ;

statement:
    var_declaration ';' {
        $$ = $1;
    }
    | print_statement {
        $$ = $1;
    }
    ;

print_statement:
    TOKEN_PRINT TOKEN_STRING_LITERAL ';' {
        ASTNode *value = create_str_node($2);
        free($2);
        $$ = create_print_node(value);
    }
    | TOKEN_PRINTV TOKEN_IDENTIFIER ';' {
        const Symbol *symbol = symbol_table_lookup(&symbol_table, $2);
        if (symbol == NULL) {
            fprintf(stderr, "Undeclared identifier: %s\n", $2);
            free($2);
            YYERROR;
        }
        ASTNode *value = create_identifier_node($2, symbol->type);
        free($2);
        $$ = create_print_node(value);
    }
    ;

var_declaration:
    TOKEN_VAR ':' TOKEN_TYPE TOKEN_IDENTIFIER '=' expression {
        DataType *type = parse_string_data_type($3);
        $$ = create_var_decl(type, $4, $6);
        free($3);
        free($4);
        if (symbol_table_add_declaration(&symbol_table, $$) != 1) {
            free_ast($$);
            YYERROR;
        }
    }
    | TOKEN_VAR ':' TOKEN_TYPE TOKEN_IDENTIFIER {
        DataType *type = parse_string_data_type($3);
        $$ = create_var_decl(type, $4, NULL);
        free($3);
        free($4);
        if (symbol_table_add_declaration(&symbol_table, $$) != 1) {
            free_ast($$);
            YYERROR;
        }
    }
    | TOKEN_CONST ':' TOKEN_TYPE TOKEN_IDENTIFIER '=' expression {
        DataType *type = parse_string_data_type($3);
        $$ = create_const_decl(type, $4, $6);
        free($3);
        free($4);
        if (symbol_table_add_declaration(&symbol_table, $$) != 1) {
            free_ast($$);
            YYERROR;
        }
    }
    ;

expression:
    TOKEN_INT_LITERAL {
        $$ = create_int_node($1);
    }
    | TOKEN_STRING_LITERAL {
        $$ = create_str_node($1);
        free($1);
    }
    | TOKEN_FLOAT_LITERAL { $$ = create_float_node($1); }
    | TOKEN_BOOL_LITERAL { $$ = create_bool_node($1); }
    | TOKEN_CHAR_LITERAL { $$ = create_char_node($1); }
    | TOKEN_IDENTIFIER {
        const Symbol *symbol = symbol_table_lookup(&symbol_table, $1);
        if (symbol == NULL) {
            fprintf(stderr, "Undeclared identifier: %s\n", $1);
            free($1);
            YYERROR;
        }
        $$ = create_identifier_node($1, symbol->type);
        free($1);
    }
    | '(' expression ')' { $$ = $2; }
    | expression '+' expression {
        if (($1->expressionType != TYPE_INT && $1->expressionType != TYPE_FLOAT) ||
            ($3->expressionType != TYPE_INT && $3->expressionType != TYPE_FLOAT)) {
            fprintf(stderr, "Addition requires numeric operands\n");
            free_ast($1);
            free_ast($3);
            YYERROR;
        }
        $$ = create_binary_node(OP_ADD, $1, $3);
    }
    | expression '-' expression {
        if (($1->expressionType != TYPE_INT && $1->expressionType != TYPE_FLOAT) ||
            ($3->expressionType != TYPE_INT && $3->expressionType != TYPE_FLOAT)) {
            fprintf(stderr, "Subtraction requires numeric operands\n");
            free_ast($1);
            free_ast($3);
            YYERROR;
        }
        $$ = create_binary_node(OP_SUB, $1, $3);
    }
    | expression '*' expression {
        if (($1->expressionType != TYPE_INT && $1->expressionType != TYPE_FLOAT) ||
            ($3->expressionType != TYPE_INT && $3->expressionType != TYPE_FLOAT)) {
            fprintf(stderr, "Multiplication requires numeric operands\n");
            free_ast($1);
            free_ast($3);
            YYERROR;
        }
        $$ = create_binary_node(OP_MUL, $1, $3);
    }
    | expression '/' expression {
        if (($1->expressionType != TYPE_INT && $1->expressionType != TYPE_FLOAT) ||
            ($3->expressionType != TYPE_INT && $3->expressionType != TYPE_FLOAT)) {
            fprintf(stderr, "Division requires numeric operands\n");
            free_ast($1);
            free_ast($3);
            YYERROR;
        }
        $$ = create_binary_node(OP_DIV, $1, $3);
    }
    ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "Parse error: %s\n", s);
}
