#ifndef AST_H
#define AST_H

typedef enum {
  TYPE_INT,
  TYPE_FLOAT,
  TYPE_CHAR,
  TYPE_BOOL,
  TYPE_STRING,
  TYPE_VOID,
  TYPE_UNKNOWN
} DataType;

typedef enum {
  OP_ADD,
  OP_SUB,
  OP_MUL,
  OP_DIV,
  /* Relational: always produce TYPE_BOOL, regardless of operand type. */
  OP_EQ,
  OP_NEQ,
  OP_LT,
  OP_GT,
  OP_LE,
  OP_GE,
  /* Logical: operands and result are TYPE_BOOL. */
  OP_AND,
  OP_OR,
} BinaryOperator;

typedef enum {
  OP_NOT,
} UnaryOperator;

typedef enum {
  //Declarations
  VAR_DECL,
  CONST_DECL,
  FUNC_DECL,

  //Literals
  INT_LITERAL,
  STR_LITERAL,
  BOOL_LITERAL,
  FLOAT_LITERAL,
  CHAR_LITERAL,
  IDENTIFIER_REF,
  BINARY_OP,
  UNARY_OP,
  PRINT_STMT,
  FUNC_CALL,
  RETURN_STMT,
  IF_STMT,
} NodeType;

typedef struct VarDeclaration {
  DataType *dataType;
  char *identifier;
  struct ASTNode *init;
} VarDeclaration;

/* Linked list of typed parameters belonging to a function declaration. */
typedef struct Parameter {
  DataType type;
  char *name;
  struct Parameter *next;
} Parameter;

typedef struct FuncDeclaration {
  DataType returnType;
  char *name;
  Parameter *params;
  struct ASTNode *body; // Linked list of statements (via ->next)
} FuncDeclaration;

typedef struct FuncCall {
  char *name;
  struct ASTNode *args; // Linked list of argument expressions (via ->next)
} FuncCall;

typedef struct ReturnStmt {
  struct ASTNode *value; // NULL for a bare "return;" in a void function
} ReturnStmt;

typedef struct IfStmt {
  struct ASTNode *condition;
  struct ASTNode *thenBranch; // Linked list of statements (via ->next)
  struct ASTNode *elseBranch; // NULL when there is no else/else-if
} IfStmt;

typedef struct ASTNode {
  NodeType type;
  DataType expressionType;
  union {
    VarDeclaration varDeclaration;
    FuncDeclaration funcDeclaration;
    FuncCall funcCall;
    ReturnStmt returnStmt;
    IfStmt ifStmt;
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
      UnaryOperator op;
      struct ASTNode *operand;
    } unary;
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
ASTNode *create_unary_node(UnaryOperator op, ASTNode *operand);
ASTNode *create_print_node(ASTNode *value);
ASTNode *create_var_decl(DataType *dtype, const char *name, ASTNode *init);
ASTNode *create_const_decl(DataType *dtype, const char *name, ASTNode *init);
Parameter *create_parameter(DataType type, const char *name);
ASTNode *create_func_decl(DataType returnType, const char *name,
                          Parameter *params, ASTNode *body);
ASTNode *create_func_call(const char *name, ASTNode *args,
                          DataType returnType);
ASTNode *create_return_stmt(ASTNode *value);
ASTNode *create_if_stmt(ASTNode *condition, ASTNode *thenBranch,
                        ASTNode *elseBranch);
void free_parameters(Parameter *params);
DataType *parse_string_data_type(char *val);
void print_ast(ASTNode *root);
void free_ast(ASTNode *node);

#endif
