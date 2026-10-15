#include "ast.h"
#include "string_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static ASTNode *allocate_new_node(void) {
  ASTNode *node = calloc(1, sizeof(*node));

  if (node == NULL) {
    fprintf(stderr, "Out of memory\n");
    exit(EXIT_FAILURE);
  }
  return node;
}

ASTNode *create_int_node(int val) {
  ASTNode *node = allocate_new_node();
  node->type = INT_LITERAL;
  node->expressionType = TYPE_INT;
  node->data.intValue = val;
  node->next = NULL;

  return node;
}

ASTNode *create_float_node(float val) {
  ASTNode *node = allocate_new_node();
  node->type = FLOAT_LITERAL;
  node->expressionType = TYPE_FLOAT;
  node->data.floatValue = val;
  node->next = NULL;

  return node;
}

ASTNode *create_bool_node(int val) {
  ASTNode *node = allocate_new_node();
  node->type = BOOL_LITERAL;
  node->expressionType = TYPE_BOOL;
  node->data.boolValue = val;
  node->next = NULL;

  return node;
}

ASTNode *create_char_node(char val) {
  ASTNode *node = allocate_new_node();
  node->type = CHAR_LITERAL;
  node->expressionType = TYPE_CHAR;
  node->data.charValue = val;
  node->next = NULL;

  return node;
}

ASTNode *create_str_node(const char *val) {
  ASTNode *node = allocate_new_node();

  node->type = STR_LITERAL;
  node->expressionType = TYPE_STRING;
  node->data.strValue = string_copy(val);
  if (node->data.strValue == NULL) {
    fprintf(stderr, "Out of memory\n");
    free(node);
    exit(EXIT_FAILURE);
  }

  return node;
}

ASTNode *create_identifier_node(const char *name, DataType type) {
  ASTNode *node = allocate_new_node();
  node->type = IDENTIFIER_REF;
  node->expressionType = type;
  node->data.identifier = string_copy(name);
  if (node->data.identifier == NULL) {
    fprintf(stderr, "Out of memory\n");
    free(node);
    exit(EXIT_FAILURE);
  }
  return node;
}

ASTNode *create_binary_node(BinaryOperator op, ASTNode *left, ASTNode *right) {
  ASTNode *node = allocate_new_node();
  node->type = BINARY_OP;
  node->expressionType =
      left->expressionType == TYPE_FLOAT || right->expressionType == TYPE_FLOAT
          ? TYPE_FLOAT
          : TYPE_INT;
  node->data.binary.op = op;
  node->data.binary.left = left;
  node->data.binary.right = right;
  return node;
}

ASTNode *create_print_node(ASTNode *value) {
  ASTNode *node = allocate_new_node();
  node->type = PRINT_STMT;
  node->data.print.value = value;
  return node;
}

ASTNode *create_var_decl(DataType *dtype, const char *name, ASTNode *init) {
  ASTNode *node = allocate_new_node();
  node->type = VAR_DECL;
  node->data.varDeclaration.dataType = dtype;
  node->data.varDeclaration.identifier = string_copy(name);
  if (node->data.varDeclaration.identifier == NULL) {
    fprintf(stderr, "Out of memory\n");
    free(node);
    exit(EXIT_FAILURE);
  }
  node->data.varDeclaration.init = init;
  return node;
}

ASTNode *create_const_decl(DataType *dtype, const char *name, ASTNode *init) {
  ASTNode *node = create_var_decl(dtype, name, init);
  node->type = CONST_DECL;
  return node;
}

Parameter *create_parameter(DataType type, const char *name) {
  Parameter *param = malloc(sizeof(*param));
  if (param == NULL) {
    fprintf(stderr, "Out of memory\n");
    exit(EXIT_FAILURE);
  }
  param->type = type;
  param->name = string_copy(name);
  if (param->name == NULL) {
    fprintf(stderr, "Out of memory\n");
    free(param);
    exit(EXIT_FAILURE);
  }
  param->next = NULL;
  return param;
}

ASTNode *create_func_decl(DataType returnType, const char *name,
                          Parameter *params, ASTNode *body) {
  ASTNode *node = allocate_new_node();
  node->type = FUNC_DECL;
  node->expressionType = returnType;
  node->data.funcDeclaration.returnType = returnType;
  node->data.funcDeclaration.name = string_copy(name);
  if (node->data.funcDeclaration.name == NULL) {
    fprintf(stderr, "Out of memory\n");
    free(node);
    exit(EXIT_FAILURE);
  }
  node->data.funcDeclaration.params = params;
  node->data.funcDeclaration.body = body;
  return node;
}

ASTNode *create_func_call(const char *name, ASTNode *args,
                          DataType returnType) {
  ASTNode *node = allocate_new_node();
  node->type = FUNC_CALL;
  node->expressionType = returnType;
  node->data.funcCall.name = string_copy(name);
  if (node->data.funcCall.name == NULL) {
    fprintf(stderr, "Out of memory\n");
    free(node);
    exit(EXIT_FAILURE);
  }
  node->data.funcCall.args = args;
  return node;
}

ASTNode *create_return_stmt(ASTNode *value) {
  ASTNode *node = allocate_new_node();
  node->type = RETURN_STMT;
  node->expressionType = value != NULL ? value->expressionType : TYPE_VOID;
  node->data.returnStmt.value = value;
  return node;
}

void free_parameters(Parameter *params) {
  while (params != NULL) {
    Parameter *next = params->next;
    free(params->name);
    free(params);
    params = next;
  }
}

DataType *parse_string_data_type(char *value) {
  DataType *type = malloc(sizeof(*type));
  if (type == NULL) {
    fprintf(stderr, "Out of memory\n");
    exit(EXIT_FAILURE);
  }

  if (value == NULL) {
    *type = TYPE_UNKNOWN;
  } else if (strcmp(value, "int") == 0) {
    *type = TYPE_INT;
  } else if (strcmp(value, "float") == 0) {
    *type = TYPE_FLOAT;
  } else if (strcmp(value, "char") == 0) {
    *type = TYPE_CHAR;
  } else if (strcmp(value, "bool") == 0) {
    *type = TYPE_BOOL;
  } else if (strcmp(value, "str") == 0 || strcmp(value, "string") == 0) {
    *type = TYPE_STRING;
  } else if (strcmp(value, "void") == 0) {
    *type = TYPE_VOID;
  } else {
    *type = TYPE_UNKNOWN;
  }
  return type;
}

void print_ast(ASTNode *node) {
  while (node != NULL) {
    if (node->type == VAR_DECL || node->type == CONST_DECL) {
      printf("%sDeclaration:\n",
             node->type == CONST_DECL ? "Const" : "Variable");
      printf("  Identifier: %s\n", node->data.varDeclaration.identifier);
      printf("  Type: ");
      if (node->data.varDeclaration.dataType != NULL) {
        switch (*node->data.varDeclaration.dataType) {
        case TYPE_INT:
          printf("int");
          break;
        case TYPE_FLOAT:
          printf("float");
          break;
        case TYPE_CHAR:
          printf("char");
          break;
        case TYPE_BOOL:
          printf("bool");
          break;
        case TYPE_STRING:
          printf("string");
          break;
        case TYPE_VOID:
          printf("void");
          break;
        case TYPE_UNKNOWN:
          printf("unknown");
          break;
        }
      } else {
        printf("unknown");
      }
      printf("\n");
      if (node->data.varDeclaration.init != NULL) {
        printf("  Initializer: ");
        print_ast(node->data.varDeclaration.init);
      }
    } else if (node->type == FUNC_DECL) {
      printf("FunctionDeclaration: %s\n", node->data.funcDeclaration.name);
      printf("  Parameters:");
      if (node->data.funcDeclaration.params == NULL) {
        printf(" (none)");
      }
      printf("\n");
      for (Parameter *param = node->data.funcDeclaration.params; param != NULL;
           param = param->next) {
        printf("    %s\n", param->name);
      }
      printf("  Body:\n");
      print_ast(node->data.funcDeclaration.body);
    } else {
      switch (node->type) {
      case INT_LITERAL:
        printf("IntegerLiteral(%d)\n", node->data.intValue);
        break;
      case FLOAT_LITERAL:
        printf("FloatLiteral(%g)\n", node->data.floatValue);
        break;
      case BOOL_LITERAL:
        printf("BooleanLiteral(%s)\n", node->data.boolValue ? "true" : "false");
        break;
      case CHAR_LITERAL:
        printf("CharacterLiteral('%c')\n", node->data.charValue);
        break;
      case STR_LITERAL:
        printf("StringLiteral(\"%s\")\n", node->data.strValue);
        break;
      case IDENTIFIER_REF:
        printf("IdentifierReference(%s)\n", node->data.identifier);
        break;
      case BINARY_OP: {
        const char *op_symbol;
        switch (node->data.binary.op) {
        case OP_ADD: op_symbol = "+"; break;
        case OP_SUB: op_symbol = "-"; break;
        case OP_MUL: op_symbol = "*"; break;
        case OP_DIV: op_symbol = "/"; break;
        default: op_symbol = "?"; break;
        }
        printf("BinaryOperation(%s)\n", op_symbol);
        printf("  Left: ");
        print_ast(node->data.binary.left);
        printf("  Right: ");
        print_ast(node->data.binary.right);
        break;
      }
      case PRINT_STMT:
        printf("PrintStatement:\n  Value: ");
        print_ast(node->data.print.value);
        break;
      case FUNC_CALL:
        printf("FunctionCall(%s)\n", node->data.funcCall.name);
        if (node->data.funcCall.args != NULL) {
          printf("  Arguments:\n");
          print_ast(node->data.funcCall.args);
        }
        break;
      case RETURN_STMT:
        printf("ReturnStatement:\n");
        if (node->data.returnStmt.value != NULL) {
          printf("  Value: ");
          print_ast(node->data.returnStmt.value);
        }
        break;
      default:
        break;
      }
    }
    node = node->next;
  }
}

void free_ast(ASTNode *node) {
  while (node != NULL) {
    /* Save next statement before freeing current node */
    ASTNode *next_node = node->next;

    switch (node->type) {
    case VAR_DECL:
    case CONST_DECL:
      /* Free duplicated strings from varDeclaration */
      if (node->data.varDeclaration.dataType) {
        free(node->data.varDeclaration.dataType);
      }
      if (node->data.varDeclaration.identifier) {
        free(node->data.varDeclaration.identifier);
      }
      /* Recursively free the initialization expression subtree */
      if (node->data.varDeclaration.init) {
        free_ast(node->data.varDeclaration.init);
      }
      break;

    case STR_LITERAL:
      /* Free the duplicated string literal */
      if (node->data.strValue) {
        free(node->data.strValue);
      }
      break;

    case IDENTIFIER_REF:
      free(node->data.identifier);
      break;

    case BINARY_OP:
      free_ast(node->data.binary.left);
      free_ast(node->data.binary.right);
      break;

    case PRINT_STMT:
      free_ast(node->data.print.value);
      break;

    case FUNC_DECL:
      free(node->data.funcDeclaration.name);
      free_parameters(node->data.funcDeclaration.params);
      if (node->data.funcDeclaration.body) {
        free_ast(node->data.funcDeclaration.body);
      }
      break;

    case FUNC_CALL:
      free(node->data.funcCall.name);
      if (node->data.funcCall.args) {
        free_ast(node->data.funcCall.args);
      }
      break;

    case RETURN_STMT:
      if (node->data.returnStmt.value) {
        free_ast(node->data.returnStmt.value);
      }
      break;

    case INT_LITERAL:
    case FLOAT_LITERAL:
    case BOOL_LITERAL:
    case CHAR_LITERAL:
      /* No dynamically allocated fields in these literals. */
      break;
    }

    /* Free the ASTNode struct itself */
    free(node);

    /* Advance to the next statement in the linked list */
    node = next_node;
  }
}
