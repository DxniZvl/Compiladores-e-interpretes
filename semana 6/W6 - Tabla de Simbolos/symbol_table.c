#include "symbol_table.h"
#include "string_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void symbol_table_init(SymbolTable *table) { table->head = NULL; }

const Symbol *symbol_table_lookup(const SymbolTable *table, const char *name) {
  for (const Symbol *symbol = table->head; symbol != NULL;
       symbol = symbol->next) {
    if (strcmp(symbol->name, name) == 0) {
      return symbol;
    }
  }
  return NULL;
}

int symbol_table_insert(SymbolTable *table, const char *name, DataType type) {
  if (symbol_table_lookup(table, name) != NULL) {
    return 0;
  }

  Symbol *symbol = malloc(sizeof(*symbol));
  if (symbol == NULL) {
    return -1;
  }
  symbol->name = string_copy(name);
  if (symbol->name == NULL) {
    free(symbol);
    return -1;
  }
  symbol->type = type;
  symbol->next = table->head;
  table->head = symbol;
  return 1;
}

int symbol_table_add_declaration(SymbolTable *table,
                                 const ASTNode *declaration) {
  const VarDeclaration *var = &declaration->data.varDeclaration;
  int result = symbol_table_insert(
      table, var->identifier,
      var->dataType != NULL ? *var->dataType : TYPE_UNKNOWN);
  if (result == 0) {
    fprintf(stderr, "Duplicate declaration: %s\n", var->identifier);
  } else if (result < 0) {
    fprintf(stderr, "Out of memory while adding symbol\n");
  }
  return result;
}

static const char *type_name(DataType type) {
  switch (type) {
  case TYPE_INT: return "int";
  case TYPE_FLOAT: return "float";
  case TYPE_CHAR: return "char";
  case TYPE_BOOL: return "bool";
  case TYPE_STRING: return "string";
  case TYPE_UNKNOWN: return "unknown";
  }
  return "unknown";
}

void symbol_table_print(const SymbolTable *table) {
  for (const Symbol *symbol = table->head; symbol != NULL;
       symbol = symbol->next) {
    printf("%s: %s\n", symbol->name, type_name(symbol->type));
  }
}

void symbol_table_destroy(SymbolTable *table) {
  Symbol *symbol = table->head;
  while (symbol != NULL) {
    Symbol *next = symbol->next;
    free(symbol->name);
    free(symbol);
    symbol = next;
  }
  table->head = NULL;
}
