#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ASTNode *allocate_new_node() {
  ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));

  // If fails in locate in memory
  if (node == NULL) {
    fprintf(stderr, "Out of memory\n");
    exit(EXIT_FAILURE);
  }
  return node;
}

ASTNode *create_int_node(int val) {
  ASTNode *node = allocate_new_node();
  node->type = INT_LITERAL;
  node->intLit.intValue = val;
  node->next = NULL;
  return node;
}

ASTNode *create_bool_node(int val) {
  ASTNode *node = allocate_new_node();
  node->type = BOOL_LITERAL;
  node->boolLit.boolValue = val;
  node->next = NULL;
  return node;
}


ASTNode *create_str_node(const char *val) {
  ASTNode *node = allocate_new_node();

  node->type = STR_LITERAL;
  node->strLit.strValue = strdup(val);
  node->next = NULL;

  return node;
}

ASTNode *create_var_decl(char *dtype, const char *name, ASTNode *init, int isConst) {

  ASTNode *node = allocate_new_node();
  node->type = VAR_DECL;
  node->varDeclaration.dataType = strdup(dtype);
  node->varDeclaration.identifier = strdup(name);
  node->varDeclaration.init = init;
  node->varDeclaration.isConst = isConst;
  node->next = NULL;
  return node;
}

void print_ast(ASTNode *node) {
  while (node != NULL) {
    if (node->type == VAR_DECL) {
      if (node->varDeclaration.isConst) {
        printf("ConstantDeclaration:\n");
      } else {
        printf("VariableDeclaration:\n");
      }
      printf("  Type: %s\n", node->varDeclaration.dataType);
      printf("  Identifier: %s\n", node->varDeclaration.identifier);
      if (node->varDeclaration.init) {
        if (node->varDeclaration.init->type == INT_LITERAL) {
          printf("  Init: IntegerLiteral(%d)\n",
                 node->varDeclaration.init->intLit.intValue);
        } else if (node->varDeclaration.init->type == STR_LITERAL) {
          printf("  Init: StringLiteral(\"%s\")\n",
                 node->varDeclaration.init->strLit.strValue);
        } else if (node->varDeclaration.init->type == BOOL_LITERAL) {
          printf("  Init: BooleanLiteral(%s)\n",
                 node->varDeclaration.init->boolLit.boolValue ? "true" : "false");
        }
      }
    }
    node = node->next;
    if (node)
      printf("\n");
  }
}

void free_ast(ASTNode *node) {
  while (node != NULL) {
    /* Save next statement before freeing current node */
    ASTNode *next_node = node->next;

    switch (node->type) {
    case VAR_DECL:
      /* Free duplicated strings from varDeclaration */
      if (node->varDeclaration.dataType) {
        free(node->varDeclaration.dataType);
      }
      if (node->varDeclaration.identifier) {
        free(node->varDeclaration.identifier);
      }
      /* Recursively free the initialization expression subtree */
      if (node->varDeclaration.init) {
        free_ast(node->varDeclaration.init);
      }
      break;

    case STR_LITERAL:
      /* Free the duplicated string literal */
      if (node->strLit.strValue) {
        free(node->strLit.strValue);
      }
      break;

    case INT_LITERAL:
      /* No dynamically allocated fields inside intLit */
      break;

    case BOOL_LITERAL:
      /* No dynamically allocated fields inside boolLit */
      break;
    }

    /* Free the ASTNode struct itself */
    free(node);

    /* Advance to the next statement in the linked list */
    node = next_node;
  }
}
