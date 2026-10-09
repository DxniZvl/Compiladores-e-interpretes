#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include "ast.h"
#include "symbol_table.h"

/* Recorre el AST ya construido por el parser, llena la tabla de simbolos
   y valida tipos, declaraciones duplicadas, variables no declaradas y
   asignaciones a pichirre. Retorna 1 si todo esta correcto, 0 si hubo
   al menos un error semantico (el mensaje ya se imprimio en stderr). */
int analyze_program(ASTNode *program, SymbolTable *table);

#endif
