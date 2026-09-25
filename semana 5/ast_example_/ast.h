#ifndef AST_H
#define AST_H

typedef enum {
  VAR_DECL,
  INT_LITERAL,
  STR_LITERAL,
  BOOL_LITERAL,
} NodeType;

typedef struct VarDeclaration {
  char *dataType;
  char *identifier;
  int isConst;
  struct ASTNode *init;
} VarDeclaration;

typedef struct ASTNode {
  NodeType type;
  union {
    struct VarDeclaration varDeclaration;
    struct {
      int intValue;
    } intLit;
    struct {
      char *strValue;
    } strLit;
    struct {
      int boolValue;
    } boolLit;
  };

  struct ASTNode *next; //Linked list for multiple statements

} ASTNode;

ASTNode* create_bool_node(int val);
ASTNode* create_int_node(int val);
ASTNode* create_str_node(const char *val);
ASTNode* create_var_decl(char *dtype, const char *name, ASTNode *init, int isConst);
void print_ast(ASTNode *root);
void free_ast(ASTNode *node);

#endif
