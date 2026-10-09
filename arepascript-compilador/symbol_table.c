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

int symbol_table_insert(SymbolTable *table, const char *name, DataType type,
                        int is_const) {
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
  symbol->is_const = is_const;
  symbol->next = table->head;
  table->head = symbol;
  return 1;
}

void symbol_table_print(const SymbolTable *table) {
  printf("%-18s %-14s %-10s\n", "Nombre", "Tipo", "Categoria");
  printf("--------------------------------------------\n");
  for (const Symbol *symbol = table->head; symbol != NULL;
       symbol = symbol->next) {
    printf("%-18s %-14s %-10s\n", symbol->name, arepa_type_name(symbol->type),
           symbol->is_const ? "pichirre" : "vaina");
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
