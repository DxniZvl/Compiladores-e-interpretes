%{
#include "ast.h"
#include "symbol_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int yylex();
void yyerror(const char *s);

ASTNode *ast_root = NULL;
SymbolTable symbol_table;
FunctionTable function_table;

/* There is no scope stack: nested function declarations are rejected, so a
 * single "are we inside a function body right now" flag together with the
 * enclosing function's declared return type is enough to validate `return`
 * statements and to know where to restore the symbol table to afterwards. */
static int in_function_body = 0;
static DataType current_function_return_type = TYPE_UNKNOWN;
static Symbol *function_scope_mark = NULL;

static int is_numeric_type(DataType type) {
    return type == TYPE_INT || type == TYPE_FLOAT;
}

static int check_binary_operand_types(const char *op_name, DataType required,
                                      DataType left, DataType right) {
    if (required == TYPE_UNKNOWN) {
        /* "numeric" check: int/float may mix freely. */
        if (!is_numeric_type(left) || !is_numeric_type(right)) {
            fprintf(stderr, "'%s' requires numeric operands\n", op_name);
            return 0;
        }
        return 1;
    }
    if (left != required || right != required) {
        fprintf(stderr, "'%s' requires %s operands\n", op_name,
                required == TYPE_BOOL ? "boolean" : "matching");
        return 0;
    }
    return 1;
}

static int check_equality_operand_types(const char *op_name, DataType left,
                                        DataType right) {
    if (is_numeric_type(left) && is_numeric_type(right)) {
        return 1;
    }
    if (left == right) {
        return 1;
    }
    fprintf(stderr, "'%s' requires operands of the same type\n", op_name);
    return 0;
}

static ASTNode *build_function_call(const char *name, ASTNode *args) {
    const FunctionSymbol *function = function_table_lookup(&function_table, name);
    if (function == NULL) {
        fprintf(stderr, "Undefined function: %s\n", name);
        free_ast(args);
        return NULL;
    }

    size_t arg_count = 0;
    for (ASTNode *arg = args; arg != NULL; arg = arg->next) {
        arg_count++;
    }
    size_t param_count = 0;
    for (const Parameter *param = function->params; param != NULL; param = param->next) {
        param_count++;
    }
    if (arg_count != param_count) {
        fprintf(stderr, "Function '%s' expects %zu argument(s), got %zu\n",
                name, param_count, arg_count);
        free_ast(args);
        return NULL;
    }

    const Parameter *param = function->params;
    ASTNode *arg = args;
    while (param != NULL) {
        int ok = (is_numeric_type(param->type) && is_numeric_type(arg->expressionType)) ||
                 param->type == arg->expressionType;
        if (!ok) {
            fprintf(stderr, "Function '%s': argument type mismatch for parameter '%s'\n",
                    name, param->name);
            free_ast(args);
            return NULL;
        }
        param = param->next;
        arg = arg->next;
    }

    return create_func_call(name, args, function->returnType);
}

%}

%union {
    int int_val;
    char *str_val;
    char char_val;
    float float_value;
    int bool_value;
    struct ASTNode *node;
    struct Parameter *param;
}

%token TOKEN_VAR TOKEN_CONST TOKEN_PRINT TOKEN_PRINTV TOKEN_FUNC TOKEN_RETURN
%token TOKEN_IF TOKEN_ELSE
%token TOKEN_EQ TOKEN_NEQ TOKEN_LE TOKEN_GE TOKEN_AND TOKEN_OR
%token <str_val> TOKEN_TYPE
%token <str_val> TOKEN_IDENTIFIER
%token <str_val> TOKEN_STRING_LITERAL
%token <int_val> TOKEN_INT_LITERAL
%token <float_value> TOKEN_FLOAT_LITERAL
%token <bool_value> TOKEN_BOOL_LITERAL
%token <char_val> TOKEN_CHAR_LITERAL

%left TOKEN_OR
%left TOKEN_AND
%left TOKEN_EQ TOKEN_NEQ
%left '<' '>' TOKEN_LE TOKEN_GE
%left '+' '-'
%left '*' '/'
%right '!'

//any of these non-terminal rules produce a result
%type <node> program statement_list statement_list_opt statement var_declaration print_statement expression
%type <node> function_declaration call_statement return_statement arg_list arg_list_opt
%type <node> if_statement else_clause_opt
%type <param> param param_list param_list_opt

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
    | function_declaration {
        $$ = $1;
    }
    | call_statement {
        $$ = $1;
    }
    | return_statement {
        $$ = $1;
    }
    | if_statement {
        $$ = $1;
    }
    ;

if_statement:
    TOKEN_IF '(' expression ')' '{' statement_list_opt '}' else_clause_opt {
        if ($3->expressionType != TYPE_BOOL) {
            fprintf(stderr, "'if' condition must be a boolean expression\n");
            free_ast($3);
            free_ast($6);
            free_ast($8);
            YYERROR;
        }
        $$ = create_if_stmt($3, $6, $8);
    }
    ;

else_clause_opt:
    /* empty */ {
        $$ = NULL;
    }
    | TOKEN_ELSE '{' statement_list_opt '}' {
        $$ = $3;
    }
    | TOKEN_ELSE if_statement {
        $$ = $2;
    }
    ;

statement_list_opt:
    /* empty */ {
        $$ = NULL;
    }
    | statement_list {
        $$ = $1;
    }
    ;

param_list_opt:
    /* empty */ {
        $$ = NULL;
    }
    | param_list {
        $$ = $1;
    }
    ;

param_list:
    param {
        $$ = $1;
    }
    | param_list ',' param {
        Parameter *cur = $1;
        while (cur->next != NULL) {
            cur = cur->next;
        }
        cur->next = $3;
        $$ = $1;
    }
    ;

param:
    TOKEN_TYPE TOKEN_IDENTIFIER {
        DataType *type = parse_string_data_type($1);
        $$ = create_parameter(*type, $2);
        free(type);
        free($1);
        free($2);
    }
    ;

arg_list_opt:
    /* empty */ {
        $$ = NULL;
    }
    | arg_list {
        $$ = $1;
    }
    ;

arg_list:
    expression {
        $$ = $1;
    }
    | arg_list ',' expression {
        ASTNode *cur = $1;
        while (cur->next != NULL) {
            cur = cur->next;
        }
        cur->next = $3;
        $$ = $1;
    }
    ;

function_declaration:
    TOKEN_FUNC TOKEN_TYPE TOKEN_IDENTIFIER '(' param_list_opt ')'
    {
        if (in_function_body) {
            fprintf(stderr, "Nested function declarations are not supported: %s\n", $3);
            free($2);
            free($3);
            free_parameters($5);
            YYERROR;
        }

        for (Parameter *p = $5; p != NULL; p = p->next) {
            for (Parameter *q = p->next; q != NULL; q = q->next) {
                if (strcmp(p->name, q->name) == 0) {
                    fprintf(stderr, "Duplicate parameter name '%s' in function '%s'\n",
                            p->name, $3);
                    free($2);
                    free($3);
                    free_parameters($5);
                    YYERROR;
                }
            }
        }

        DataType *returnType = parse_string_data_type($2);
        free($2);

        if (function_table_add(&function_table, $3, *returnType, $5) != 1) {
            fprintf(stderr, "Duplicate function declaration: %s\n", $3);
            free($3);
            free(returnType);
            free_parameters($5);
            YYERROR;
        }

        current_function_return_type = *returnType;
        free(returnType);
        in_function_body = 1;
        function_scope_mark = symbol_table_mark(&symbol_table);
        for (Parameter *p = $5; p != NULL; p = p->next) {
            if (symbol_table_insert_scoped(&symbol_table, p->name, p->type) < 0) {
                fprintf(stderr, "Out of memory while binding parameter '%s'\n", p->name);
                YYERROR;
            }
        }
    }
    '{' statement_list_opt '}'
    {
        in_function_body = 0;
        symbol_table_truncate(&symbol_table, function_scope_mark);
        $$ = create_func_decl(current_function_return_type, $3, $5, $9);
        free($3);
    }
    ;

call_statement:
    TOKEN_IDENTIFIER '(' arg_list_opt ')' ';' {
        $$ = build_function_call($1, $3);
        free($1);
        if ($$ == NULL) {
            YYERROR;
        }
    }
    ;

return_statement:
    TOKEN_RETURN ';' {
        if (!in_function_body) {
            fprintf(stderr, "'return' used outside of a function\n");
            YYERROR;
        }
        if (current_function_return_type != TYPE_VOID) {
            fprintf(stderr, "Function must return a value of its declared type\n");
            YYERROR;
        }
        $$ = create_return_stmt(NULL);
    }
    | TOKEN_RETURN expression ';' {
        if (!in_function_body) {
            fprintf(stderr, "'return' used outside of a function\n");
            free_ast($2);
            YYERROR;
        }
        int ok = (is_numeric_type(current_function_return_type) && is_numeric_type($2->expressionType)) ||
                 current_function_return_type == $2->expressionType;
        if (!ok) {
            fprintf(stderr, "Return type does not match function's declared return type\n");
            free_ast($2);
            YYERROR;
        }
        $$ = create_return_stmt($2);
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
    | TOKEN_IDENTIFIER '(' arg_list_opt ')' {
        $$ = build_function_call($1, $3);
        free($1);
        if ($$ == NULL) {
            YYERROR;
        }
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
    | expression TOKEN_EQ expression {
        if (!check_equality_operand_types("==", $1->expressionType, $3->expressionType)) {
            free_ast($1);
            free_ast($3);
            YYERROR;
        }
        $$ = create_binary_node(OP_EQ, $1, $3);
    }
    | expression TOKEN_NEQ expression {
        if (!check_equality_operand_types("!=", $1->expressionType, $3->expressionType)) {
            free_ast($1);
            free_ast($3);
            YYERROR;
        }
        $$ = create_binary_node(OP_NEQ, $1, $3);
    }
    | expression '<' expression {
        if (!check_binary_operand_types("<", TYPE_UNKNOWN, $1->expressionType, $3->expressionType)) {
            free_ast($1);
            free_ast($3);
            YYERROR;
        }
        $$ = create_binary_node(OP_LT, $1, $3);
    }
    | expression '>' expression {
        if (!check_binary_operand_types(">", TYPE_UNKNOWN, $1->expressionType, $3->expressionType)) {
            free_ast($1);
            free_ast($3);
            YYERROR;
        }
        $$ = create_binary_node(OP_GT, $1, $3);
    }
    | expression TOKEN_LE expression {
        if (!check_binary_operand_types("<=", TYPE_UNKNOWN, $1->expressionType, $3->expressionType)) {
            free_ast($1);
            free_ast($3);
            YYERROR;
        }
        $$ = create_binary_node(OP_LE, $1, $3);
    }
    | expression TOKEN_GE expression {
        if (!check_binary_operand_types(">=", TYPE_UNKNOWN, $1->expressionType, $3->expressionType)) {
            free_ast($1);
            free_ast($3);
            YYERROR;
        }
        $$ = create_binary_node(OP_GE, $1, $3);
    }
    | expression TOKEN_AND expression {
        if (!check_binary_operand_types("&&", TYPE_BOOL, $1->expressionType, $3->expressionType)) {
            free_ast($1);
            free_ast($3);
            YYERROR;
        }
        $$ = create_binary_node(OP_AND, $1, $3);
    }
    | expression TOKEN_OR expression {
        if (!check_binary_operand_types("||", TYPE_BOOL, $1->expressionType, $3->expressionType)) {
            free_ast($1);
            free_ast($3);
            YYERROR;
        }
        $$ = create_binary_node(OP_OR, $1, $3);
    }
    | '!' expression {
        if ($2->expressionType != TYPE_BOOL) {
            fprintf(stderr, "'!' requires a boolean operand\n");
            free_ast($2);
            YYERROR;
        }
        $$ = create_unary_node(OP_NOT, $2);
    }
    ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "Parse error: %s\n", s);
}
