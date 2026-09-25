#include "ast.h"
#include <stdio.h>
#include <stdlib.h>

extern FILE *yyin;               // Flex input file pointer
extern int yyparse(void);        // Bison parse entry point
extern ASTNode *ast_root;        // Root node populated by parser.y

static int read_input(int argc, char **argv) {
  if (argc > 1) {
    FILE *file = fopen(argv[1], "r");
    if (!file) {
      perror("Error opening the file");
      return -1;
    }
    yyin = file;
  } else {
    yyin = stdin;
    printf("Reading from standard input (Ctrl+D to end)...\n");
  }
  return 0;
}

static void cleanup_lexer_input(void) {
  /* Clean up: close yyin only if it was opened from a file */
  if (yyin && yyin != stdin) {
    fclose(yyin);
    yyin = NULL;
  }
}

int main(int argc, char **argv) {
  if (read_input(argc, argv) != 0) {
    return EXIT_FAILURE;
  }

  /* Run the Bison parser */
  int parse_result = yyparse();

  /* Close input stream as soon as parsing completes */
  cleanup_lexer_input();

  if (parse_result == 0) {
    printf("--- AST Successfully Built ---\n");
    print_ast(ast_root);
    
    /* Clean up the AST heap allocations */
    free_ast(ast_root);
    return EXIT_SUCCESS;
  } else {
    fprintf(stderr, "Parsing failed due to syntax errors.\n");
    return EXIT_FAILURE;
  }
}
