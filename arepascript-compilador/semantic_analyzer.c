#include "semantic_analyzer.h"

#include <stdio.h>
#include <stdlib.h>

/* ta_completo (int) puede ensancharse a quebrao (float), igual que en C.
   Para todo lo demas se exige el mismo tipo exacto. */
static int types_compatible(DataType declared, DataType actual) {
  if (declared == actual) {
    return 1;
  }
  return declared == TYPE_FLOAT && actual == TYPE_INT;
}

static DataType analyze_expression(ASTNode *expr, SymbolTable *table) {
  switch (expr->type) {
  case INT_LITERAL:
    expr->expressionType = TYPE_INT;
    return TYPE_INT;
  case FLOAT_LITERAL:
    expr->expressionType = TYPE_FLOAT;
    return TYPE_FLOAT;
  case BOOL_LITERAL:
    expr->expressionType = TYPE_BOOL;
    return TYPE_BOOL;
  case CHAR_LITERAL:
    expr->expressionType = TYPE_CHAR;
    return TYPE_CHAR;
  case STR_LITERAL:
    expr->expressionType = TYPE_STRING;
    return TYPE_STRING;

  case IDENTIFIER_REF: {
    const Symbol *symbol = symbol_table_lookup(table, expr->data.identifier);
    if (symbol == NULL) {
      fprintf(stderr, "¿Y esa vaina de dónde salió? -> '%s' no existe.\n",
              expr->data.identifier);
      return TYPE_UNKNOWN;
    }
    expr->expressionType = symbol->type;
    return symbol->type;
  }

  case BINARY_OP: {
    DataType left = analyze_expression(expr->data.binary.left, table);
    DataType right = analyze_expression(expr->data.binary.right, table);
    if (left == TYPE_UNKNOWN || right == TYPE_UNKNOWN) {
      return TYPE_UNKNOWN;
    }
    if ((left != TYPE_INT && left != TYPE_FLOAT) ||
        (right != TYPE_INT && right != TYPE_FLOAT)) {
      fprintf(stderr,
              "Epa, chamo, eso no es ta_completo ni quebrao -> la operación "
              "necesita numeros.\n");
      return TYPE_UNKNOWN;
    }
    DataType result = (left == TYPE_FLOAT || right == TYPE_FLOAT)
                           ? TYPE_FLOAT
                           : TYPE_INT;
    expr->expressionType = result;
    return result;
  }

  default:
    fprintf(stderr, "Nodo de expresion invalido en el analisis semantico\n");
    return TYPE_UNKNOWN;
  }
}

static int analyze_declaration(ASTNode *decl, SymbolTable *table) {
  VarDeclaration *var = &decl->data.varDeclaration;
  DataType declared = var->dataType != NULL ? *var->dataType : TYPE_UNKNOWN;

  DataType init_type = TYPE_UNKNOWN;
  if (var->init != NULL) {
    init_type = analyze_expression(var->init, table);
    if (init_type == TYPE_UNKNOWN) {
      return 0;
    }
  }

  if (declared == TYPE_UNKNOWN) {
    /* "vaina x = valor;" sin tipo explicito: se infiere del valor. */
    declared = init_type;
    DataType *inferred = malloc(sizeof(*inferred));
    if (inferred == NULL) {
      fprintf(stderr, "Sin memoria al inferir el tipo de '%s'\n",
              var->identifier);
      return 0;
    }
    *inferred = declared;
    var->dataType = inferred;
  } else if (var->init != NULL && !types_compatible(declared, init_type)) {
    fprintf(stderr, "Epa, chamo, eso no es %s -> '%s' esperaba otro tipo.\n",
            arepa_type_name(declared), var->identifier);
    return 0;
  }

  int is_const = decl->type == CONST_DECL;
  int inserted = symbol_table_insert(table, var->identifier, declared, is_const);
  if (inserted == 0) {
    fprintf(stderr,
            "Esa vaina ya existe -> '%s' no se puede declarar dos veces.\n",
            var->identifier);
    return 0;
  }
  if (inserted < 0) {
    fprintf(stderr, "Sin memoria mientras se guardaba '%s'\n", var->identifier);
    return 0;
  }

  decl->expressionType = declared;
  return 1;
}

static int analyze_assignment(ASTNode *stmt, SymbolTable *table) {
  const char *name = stmt->data.assignment.identifier;
  const Symbol *symbol = symbol_table_lookup(table, name);
  if (symbol == NULL) {
    fprintf(stderr, "¿Y esa vaina de dónde salió? -> '%s' no existe.\n", name);
    return 0;
  }
  if (symbol->is_const) {
    fprintf(stderr,
            "Ese pichirre no suelta el valor -> '%s' no se puede cambiar.\n",
            name);
    return 0;
  }

  DataType value_type = analyze_expression(stmt->data.assignment.value, table);
  if (value_type == TYPE_UNKNOWN) {
    return 0;
  }
  if (!types_compatible(symbol->type, value_type)) {
    fprintf(stderr, "Epa, chamo, eso no es %s -> '%s' esperaba otro tipo.\n",
            arepa_type_name(symbol->type), name);
    return 0;
  }

  stmt->expressionType = symbol->type;
  return 1;
}

static int analyze_echale(ASTNode *stmt, SymbolTable *table) {
  DataType type = analyze_expression(stmt->data.print.value, table);
  return type != TYPE_UNKNOWN;
}

int analyze_program(ASTNode *program, SymbolTable *table) {
  for (ASTNode *stmt = program; stmt != NULL; stmt = stmt->next) {
    int ok;
    switch (stmt->type) {
    case VAR_DECL:
    case CONST_DECL:
      ok = analyze_declaration(stmt, table);
      break;
    case ASSIGN_STMT:
      ok = analyze_assignment(stmt, table);
      break;
    case PRINT_STMT:
      ok = analyze_echale(stmt, table);
      break;
    default:
      fprintf(stderr, "Sentencia desconocida en el analisis semantico\n");
      ok = 0;
      break;
    }
    if (!ok) {
      return 0;
    }
  }
  return 1;
}
