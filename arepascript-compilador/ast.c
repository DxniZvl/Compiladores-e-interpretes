#include "ast.h"
#include "string_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static ASTNode *allocate_new_node(void) {
  ASTNode *node = calloc(1, sizeof(*node));

  if (node == NULL) {
    fprintf(stderr, "Sin memoria\n");
    exit(EXIT_FAILURE);
  }
  node->table_id = -1;
  node->expressionType = TYPE_UNKNOWN;
  return node;
}

ASTNode *create_int_node(int val) {
  ASTNode *node = allocate_new_node();
  node->type = INT_LITERAL;
  node->expressionType = TYPE_INT;
  node->data.intValue = val;
  return node;
}

ASTNode *create_float_node(float val) {
  ASTNode *node = allocate_new_node();
  node->type = FLOAT_LITERAL;
  node->expressionType = TYPE_FLOAT;
  node->data.floatValue = val;
  return node;
}

ASTNode *create_bool_node(int val) {
  ASTNode *node = allocate_new_node();
  node->type = BOOL_LITERAL;
  node->expressionType = TYPE_BOOL;
  node->data.boolValue = val;
  return node;
}

ASTNode *create_char_node(char val) {
  ASTNode *node = allocate_new_node();
  node->type = CHAR_LITERAL;
  node->expressionType = TYPE_CHAR;
  node->data.charValue = val;
  return node;
}

ASTNode *create_str_node(const char *val) {
  ASTNode *node = allocate_new_node();

  node->type = STR_LITERAL;
  node->expressionType = TYPE_STRING;
  node->data.strValue = string_copy(val);
  if (node->data.strValue == NULL) {
    fprintf(stderr, "Sin memoria\n");
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
    fprintf(stderr, "Sin memoria\n");
    free(node);
    exit(EXIT_FAILURE);
  }
  return node;
}

ASTNode *create_binary_node(BinaryOperator op, ASTNode *left, ASTNode *right) {
  ASTNode *node = allocate_new_node();
  node->type = BINARY_OP;
  /* El tipo real se calcula despues, en el analizador semantico,
     porque aqui todavia no sabemos el tipo de los identificadores. */
  node->expressionType = TYPE_UNKNOWN;
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

ASTNode *create_assign_node(const char *identifier, ASTNode *value) {
  ASTNode *node = allocate_new_node();
  node->type = ASSIGN_STMT;
  node->data.assignment.identifier = string_copy(identifier);
  if (node->data.assignment.identifier == NULL) {
    fprintf(stderr, "Sin memoria\n");
    free(node);
    exit(EXIT_FAILURE);
  }
  node->data.assignment.value = value;
  return node;
}

ASTNode *create_var_decl(DataType *dtype, const char *name, ASTNode *init) {
  ASTNode *node = allocate_new_node();
  node->type = VAR_DECL;
  node->data.varDeclaration.dataType = dtype;
  node->data.varDeclaration.identifier = string_copy(name);
  if (node->data.varDeclaration.identifier == NULL) {
    fprintf(stderr, "Sin memoria\n");
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

DataType *parse_string_data_type(char *value) {
  DataType *type = malloc(sizeof(*type));
  if (type == NULL) {
    fprintf(stderr, "Sin memoria\n");
    exit(EXIT_FAILURE);
  }

  if (value == NULL) {
    *type = TYPE_UNKNOWN;
  } else if (strcmp(value, "ta_completo") == 0) {
    *type = TYPE_INT;
  } else if (strcmp(value, "quebrao") == 0) {
    *type = TYPE_FLOAT;
  } else if (strcmp(value, "letra") == 0) {
    *type = TYPE_CHAR;
  } else if (strcmp(value, "si_o_no") == 0) {
    *type = TYPE_BOOL;
  } else if (strcmp(value, "cuento") == 0) {
    *type = TYPE_STRING;
  } else {
    *type = TYPE_UNKNOWN;
  }
  return type;
}

const char *arepa_type_name(DataType type) {
  switch (type) {
  case TYPE_INT: return "ta_completo";
  case TYPE_FLOAT: return "quebrao";
  case TYPE_CHAR: return "letra";
  case TYPE_BOOL: return "si_o_no";
  case TYPE_STRING: return "cuento";
  case TYPE_UNKNOWN: return "desconocido";
  }
  return "desconocido";
}

static const char *node_type_name(NodeType type) {
  switch (type) {
  case VAR_DECL: return "VAR_DECL (vaina)";
  case CONST_DECL: return "CONST_DECL (pichirre)";
  case ASSIGN_STMT: return "ASSIGN_STMT";
  case INT_LITERAL: return "INT_LITERAL";
  case STR_LITERAL: return "STR_LITERAL";
  case BOOL_LITERAL: return "BOOL_LITERAL";
  case FLOAT_LITERAL: return "FLOAT_LITERAL";
  case CHAR_LITERAL: return "CHAR_LITERAL";
  case IDENTIFIER_REF: return "IDENTIFIER_REF";
  case BINARY_OP: return "BINARY_OP";
  case PRINT_STMT: return "PRINT_STMT (echale)";
  }
  return "DESCONOCIDO";
}

static const char *op_symbol(BinaryOperator op) {
  switch (op) {
  case OP_ADD: return "+";
  case OP_SUB: return "-";
  case OP_MUL: return "*";
  case OP_DIV: return "/";
  }
  return "?";
}

void print_ast(ASTNode *node) {
  while (node != NULL) {
    if (node->type == VAR_DECL || node->type == CONST_DECL) {
      printf("%s: %s", node->type == CONST_DECL ? "pichirre" : "vaina",
             node->data.varDeclaration.identifier);
      if (node->data.varDeclaration.dataType != NULL) {
        printf(" (%s)", arepa_type_name(*node->data.varDeclaration.dataType));
      }
      printf("\n");
      if (node->data.varDeclaration.init != NULL) {
        printf("  Valor inicial: ");
        print_ast(node->data.varDeclaration.init);
      }
    } else {
      switch (node->type) {
      case ASSIGN_STMT:
        printf("Asignacion(%s)\n", node->data.assignment.identifier);
        printf("  Valor: ");
        print_ast(node->data.assignment.value);
        break;
      case INT_LITERAL:
        printf("LiteralTaCompleto(%d)\n", node->data.intValue);
        break;
      case FLOAT_LITERAL:
        printf("LiteralQuebrao(%g)\n", node->data.floatValue);
        break;
      case BOOL_LITERAL:
        printf("LiteralSiONo(%s)\n",
               node->data.boolValue ? "chevere" : "nada_que_ver");
        break;
      case CHAR_LITERAL:
        printf("LiteralLetra('%c')\n", node->data.charValue);
        break;
      case STR_LITERAL:
        printf("LiteralCuento(\"%s\")\n", node->data.strValue);
        break;
      case IDENTIFIER_REF:
        printf("Identificador(%s)\n", node->data.identifier);
        break;
      case BINARY_OP:
        printf("Operacion(%s)\n", op_symbol(node->data.binary.op));
        printf("  Izquierda: ");
        print_ast(node->data.binary.left);
        printf("  Derecha: ");
        print_ast(node->data.binary.right);
        break;
      case PRINT_STMT:
        printf("Echale:\n  Valor: ");
        print_ast(node->data.print.value);
        break;
      default:
        break;
      }
    }
    node = node->next;
  }
}

/* --- Tabla AST --- */

static int next_table_id;

static void assign_table_ids(ASTNode *node) {
  for (ASTNode *n = node; n != NULL; n = n->next) {
    n->table_id = next_table_id++;
    switch (n->type) {
    case VAR_DECL:
    case CONST_DECL:
      if (n->data.varDeclaration.init != NULL) {
        assign_table_ids(n->data.varDeclaration.init);
      }
      break;
    case ASSIGN_STMT:
      assign_table_ids(n->data.assignment.value);
      break;
    case BINARY_OP:
      assign_table_ids(n->data.binary.left);
      assign_table_ids(n->data.binary.right);
      break;
    case PRINT_STMT:
      assign_table_ids(n->data.print.value);
      break;
    default:
      break;
    }
  }
}

static void describe_node(const ASTNode *n, char *buffer, size_t size) {
  switch (n->type) {
  case VAR_DECL:
  case CONST_DECL:
    snprintf(buffer, size, "%s%s%s",
             n->data.varDeclaration.identifier,
             n->data.varDeclaration.dataType != NULL ? " : " : " (tipo inferido)",
             n->data.varDeclaration.dataType != NULL
                 ? arepa_type_name(*n->data.varDeclaration.dataType)
                 : "");
    break;
  case ASSIGN_STMT:
    snprintf(buffer, size, "%s = ...", n->data.assignment.identifier);
    break;
  case INT_LITERAL:
    snprintf(buffer, size, "%d", n->data.intValue);
    break;
  case FLOAT_LITERAL:
    snprintf(buffer, size, "%g", n->data.floatValue);
    break;
  case BOOL_LITERAL:
    snprintf(buffer, size, "%s", n->data.boolValue ? "chevere" : "nada_que_ver");
    break;
  case CHAR_LITERAL:
    snprintf(buffer, size, "'%c'", n->data.charValue);
    break;
  case STR_LITERAL:
    snprintf(buffer, size, "\"%s\"", n->data.strValue);
    break;
  case IDENTIFIER_REF:
    snprintf(buffer, size, "%s", n->data.identifier);
    break;
  case BINARY_OP:
    snprintf(buffer, size, "operador %s", op_symbol(n->data.binary.op));
    break;
  case PRINT_STMT:
    snprintf(buffer, size, "echale ...");
    break;
  default:
    snprintf(buffer, size, "-");
    break;
  }
}

static void print_table_rows(ASTNode *node) {
  for (ASTNode *n = node; n != NULL; n = n->next) {
    char info[96];
    describe_node(n, info, sizeof(info));

    int left_id = -1;
    int right_id = -1;
    switch (n->type) {
    case VAR_DECL:
    case CONST_DECL:
      left_id = n->data.varDeclaration.init != NULL
                    ? n->data.varDeclaration.init->table_id
                    : -1;
      break;
    case ASSIGN_STMT:
      left_id = n->data.assignment.value->table_id;
      break;
    case BINARY_OP:
      left_id = n->data.binary.left->table_id;
      right_id = n->data.binary.right->table_id;
      break;
    case PRINT_STMT:
      left_id = n->data.print.value->table_id;
      break;
    default:
      break;
    }

    char left_str[8];
    char right_str[8];
    if (left_id == -1) snprintf(left_str, sizeof(left_str), "-");
    else snprintf(left_str, sizeof(left_str), "%d", left_id);
    if (right_id == -1) snprintf(right_str, sizeof(right_str), "-");
    else snprintf(right_str, sizeof(right_str), "%d", right_id);

    printf("%-4d %-22s %-28s %-6s %-6s\n", n->table_id, node_type_name(n->type),
           info, left_str, right_str);

    switch (n->type) {
    case VAR_DECL:
    case CONST_DECL:
      if (n->data.varDeclaration.init != NULL) {
        print_table_rows(n->data.varDeclaration.init);
      }
      break;
    case ASSIGN_STMT:
      print_table_rows(n->data.assignment.value);
      break;
    case BINARY_OP:
      print_table_rows(n->data.binary.left);
      print_table_rows(n->data.binary.right);
      break;
    case PRINT_STMT:
      print_table_rows(n->data.print.value);
      break;
    default:
      break;
    }
  }
}

void print_ast_table(ASTNode *root) {
  next_table_id = 1;
  assign_table_ids(root);
  printf("%-4s %-22s %-28s %-6s %-6s\n", "ID", "Nodo", "Detalle", "Izq", "Der");
  printf("------------------------------------------------------------------------\n");
  print_table_rows(root);
}

void free_ast(ASTNode *node) {
  while (node != NULL) {
    ASTNode *next_node = node->next;

    switch (node->type) {
    case VAR_DECL:
    case CONST_DECL:
      if (node->data.varDeclaration.dataType) {
        free(node->data.varDeclaration.dataType);
      }
      if (node->data.varDeclaration.identifier) {
        free(node->data.varDeclaration.identifier);
      }
      if (node->data.varDeclaration.init) {
        free_ast(node->data.varDeclaration.init);
      }
      break;

    case ASSIGN_STMT:
      free(node->data.assignment.identifier);
      free_ast(node->data.assignment.value);
      break;

    case STR_LITERAL:
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

    case INT_LITERAL:
    case FLOAT_LITERAL:
    case BOOL_LITERAL:
    case CHAR_LITERAL:
      break;
    }

    free(node);
    node = next_node;
  }
}
