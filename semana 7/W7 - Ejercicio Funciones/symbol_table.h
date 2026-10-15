#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "ast.h"

typedef struct Symbol {
  char *name;
  DataType type;
  struct Symbol *next;
} Symbol;

typedef struct {
  Symbol *head;
} SymbolTable;

/* Registry of declared functions, keyed by name. The `params` list is owned
 * by the function's FUNC_DECL AST node, not by this table. */
typedef struct FunctionSymbol {
  char *name;
  DataType returnType;
  const Parameter *params;
  struct FunctionSymbol *next;
} FunctionSymbol;

typedef struct {
  FunctionSymbol *head;
} FunctionTable;

void symbol_table_init(SymbolTable *table);
/* Returns 1 when inserted, 0 for a duplicate, and -1 on allocation failure. */
int symbol_table_insert(SymbolTable *table, const char *name, DataType type);
/* Like symbol_table_insert, but always inserts (even over an existing name),
 * so that function parameters/locals may shadow an outer variable. Returns 1
 * on success, -1 on allocation failure. Pair with symbol_table_truncate. */
int symbol_table_insert_scoped(SymbolTable *table, const char *name, DataType type);
int symbol_table_add_declaration(SymbolTable *table,
                                 const ASTNode *declaration);
const Symbol *symbol_table_lookup(const SymbolTable *table, const char *name);
void symbol_table_print(const SymbolTable *table);
void symbol_table_destroy(SymbolTable *table);

/* Snapshot/restore support used to give function parameters and locals their
 * own scope: mark before entering a function body, truncate after leaving
 * it to discard everything declared since. */
Symbol *symbol_table_mark(const SymbolTable *table);
void symbol_table_truncate(SymbolTable *table, Symbol *mark);

void function_table_init(FunctionTable *table);
/* Returns 1 when inserted, 0 for a duplicate, and -1 on allocation failure. */
int function_table_add(FunctionTable *table, const char *name,
                       DataType returnType, const Parameter *params);
const FunctionSymbol *function_table_lookup(const FunctionTable *table,
                                            const char *name);
void function_table_destroy(FunctionTable *table);

#endif
