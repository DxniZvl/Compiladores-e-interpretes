#include "ast.h"
#include "runtime.h"
#include "semantic_analyzer.h"
#include "symbol_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern FILE *yyin;
extern int yyparse(void);
extern ASTNode *ast_root;
extern SymbolTable symbol_table;

typedef struct {
  const char *input_path;
  int debug;
} Options;

static int parse_arguments(int argc, char **argv, Options *options) {
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-d") == 0) {
      options->debug = 1;
      continue;
    }
    if (argv[i][0] == '-' || options->input_path != NULL) {
      return 0;
    }
    options->input_path = argv[i];
  }
  return 1;
}

static int read_input(const char *input_path) {
  if (input_path != NULL) {
    FILE *file = fopen(input_path, "r");
    if (!file) {
      perror("Error abriendo el archivo");
      return -1;
    }
    yyin = file;
  } else {
    yyin = stdin;
    printf("Leyendo desde entrada estandar (Ctrl+D para terminar)...\n");
  }
  return 0;
}

static void print_usage(const char *program_name) {
  fprintf(stderr, "Uso: %s [-d] [archivo-fuente]\n", program_name);
}

static void cleanup_lexer_input(void) {
  if (yyin && yyin != stdin) {
    fclose(yyin);
    yyin = NULL;
  }
}

int main(int argc, char **argv) {
  Options options = {0};
  if (!parse_arguments(argc, argv, &options)) {
    print_usage(argv[0]);
    return EXIT_FAILURE;
  }

  symbol_table_init(&symbol_table);
  if (read_input(options.input_path) != 0) {
    symbol_table_destroy(&symbol_table);
    return EXIT_FAILURE;
  }

  /* 1. Analisis lexico + sintactico (bison arma el AST) */
  int parse_result = yyparse();
  cleanup_lexer_input();

  if (parse_result != 0) {
    fprintf(stderr, "La compilacion fallo: errores de sintaxis.\n");
    free_ast(ast_root);
    symbol_table_destroy(&symbol_table);
    return EXIT_FAILURE;
  }

  /* 2. Analisis semantico (llena la tabla de simbolos y valida tipos) */
  if (!analyze_program(ast_root, &symbol_table)) {
    fprintf(stderr, "La compilacion fallo: errores semanticos.\n");
    free_ast(ast_root);
    symbol_table_destroy(&symbol_table);
    return EXIT_FAILURE;
  }

  if (options.debug) {
    printf("--- Tabla AST ---\n");
    print_ast_table(ast_root);
    printf("\n--- AST (arbol) ---\n");
    print_ast(ast_root);
    printf("\n--- Tabla de Simbolos ---\n");
    symbol_table_print(&symbol_table);
    printf("\n--- Salida del programa ---\n");
  }

  /* 3. Ejecucion */
  int execution_succeeded = execute_program(ast_root);

  free_ast(ast_root);
  symbol_table_destroy(&symbol_table);
  return execution_succeeded ? EXIT_SUCCESS : EXIT_FAILURE;
}
