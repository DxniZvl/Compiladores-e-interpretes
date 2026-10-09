#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "ast.h"

typedef struct Symbol {
  char *name;
  DataType type;
  int is_const; /* 1 = pichirre, 0 = vaina */
  struct Symbol *next;
} Symbol;

typedef struct {
  Symbol *head;
} SymbolTable;

void symbol_table_init(SymbolTable *table);
/* Retorna 1 si se inserto, 0 si el nombre ya existia, -1 si no hay memoria. */
int symbol_table_insert(SymbolTable *table, const char *name, DataType type,
                        int is_const);
const Symbol *symbol_table_lookup(const SymbolTable *table, const char *name);
void symbol_table_print(const SymbolTable *table);
void symbol_table_destroy(SymbolTable *table);

#endif
