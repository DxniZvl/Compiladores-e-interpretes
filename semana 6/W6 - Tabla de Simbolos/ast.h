#ifndef AST_H
#define AST_H

typedef enum {
  TYPE_INT,
  TYPE_FLOAT,
  TYPE_CHAR,
  TYPE_BOOL,
  TYPE_STRING,
  TYPE_UNKNOWN
} DataType;

typedef enum {
  OP_ADD,
  OP_SUB,
  OP_MUL,
  OP_DIV,
} BinaryOperator;

typedef enum {
  //Declarations
  VAR_DECL,
  CONST_DECL,

  //Literals
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
  DataType *dataType;
  char *identifier;
  struct ASTNode *init;
} VarDeclaration;

typedef struct ASTNode {
  NodeType type;
  DataType expressionType;
  union {
    VarDeclaration varDeclaration;
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

  struct ASTNode *next; // Linked list for multiple statements

} ASTNode;

ASTNode *create_int_node(int val);
ASTNode *create_bool_node(int val);
ASTNode *create_float_node(float val);
ASTNode *create_str_node(const char *val);
ASTNode *create_char_node(const char val);
ASTNode *create_identifier_node(const char *name, DataType type);
ASTNode *create_binary_node(BinaryOperator op, ASTNode *left, ASTNode *right);
ASTNode *create_print_node(ASTNode *value);
ASTNode *create_var_decl(DataType *dtype, const char *name, ASTNode *init);
ASTNode *create_const_decl(DataType *dtype, const char *name, ASTNode *init);
DataType *parse_string_data_type(char *val);
void print_ast(ASTNode *root);
void free_ast(ASTNode *node);

#endif
