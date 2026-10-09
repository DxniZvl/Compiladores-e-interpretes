#ifndef AST_H
#define AST_H

typedef enum {
  TYPE_INT,    /* ta_completo */
  TYPE_FLOAT,  /* quebrao */
  TYPE_CHAR,   /* letra */
  TYPE_BOOL,   /* si_o_no */
  TYPE_STRING, /* cuento */
  TYPE_UNKNOWN
} DataType;

typedef enum {
  OP_ADD,
  OP_SUB,
  OP_MUL,
  OP_DIV,
} BinaryOperator;

typedef enum {
  /* Declaraciones */
  VAR_DECL,
  CONST_DECL,
  ASSIGN_STMT,

  /* Literales */
  INT_LITERAL,
  STR_LITERAL,
  BOOL_LITERAL,
  FLOAT_LITERAL,
  CHAR_LITERAL,
  IDENTIFIER_REF,
  BINARY_OP,
  PRINT_STMT,
} NodeType;

typedef struct VarDeclaration {
  DataType *dataType; /* NULL cuando el tipo se infiere del valor inicial */
  char *identifier;
  struct ASTNode *init;
} VarDeclaration;

typedef struct Assignment {
  char *identifier;
  struct ASTNode *value;
} Assignment;

typedef struct ASTNode {
  NodeType type;
  DataType expressionType;
  int table_id; /* usado solo por print_ast_table */
  union {
    VarDeclaration varDeclaration;
    Assignment assignment;
    int intValue;
    char *strValue;
    char charValue;
    int boolValue;
    float floatValue;
    char *identifier;
    struct {
      BinaryOperator op;
      struct ASTNode *left;
      struct ASTNode *right;
    } binary;
    struct {
      struct ASTNode *value;
    } print;
  } data;

  struct ASTNode *next; /* Lista enlazada para varias sentencias */

} ASTNode;

ASTNode *create_int_node(int val);
ASTNode *create_bool_node(int val);
ASTNode *create_float_node(float val);
ASTNode *create_str_node(const char *val);
ASTNode *create_char_node(const char val);
ASTNode *create_identifier_node(const char *name, DataType type);
ASTNode *create_binary_node(BinaryOperator op, ASTNode *left, ASTNode *right);
ASTNode *create_print_node(ASTNode *value);
ASTNode *create_assign_node(const char *identifier, ASTNode *value);
ASTNode *create_var_decl(DataType *dtype, const char *name, ASTNode *init);
ASTNode *create_const_decl(DataType *dtype, const char *name, ASTNode *init);
DataType *parse_string_data_type(char *val);
const char *arepa_type_name(DataType type);
void print_ast(ASTNode *root);
void print_ast_table(ASTNode *root);
void free_ast(ASTNode *node);

#endif
