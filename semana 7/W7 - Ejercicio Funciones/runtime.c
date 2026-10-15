#include "runtime.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  DataType type;
  union {
    int int_value;
    float float_value;
    char char_value;
    int bool_value;
    const char *string_value;
  } data;
} RuntimeValue;

typedef struct RuntimeVariable {
  const char *name;
  RuntimeValue value;
  int initialized;
  struct RuntimeVariable *next;
} RuntimeVariable;

/* Functions are registered once, up front, from the top-level FUNC_DECL
 * nodes, so that calls may appear anywhere relative to the declaration.
 * `params` and `body` are borrowed from the AST, which stays alive for the
 * whole run. */
typedef struct RuntimeFunction {
  const char *name;
  const Parameter *params;
  const ASTNode *body;
  DataType returnType;
  struct RuntimeFunction *next;
} RuntimeFunction;

/* Result of running a statement list: whether a `return` fired, and with
 * what value. */
typedef struct {
  int returned;
  RuntimeValue value;
} ExecResult;

static RuntimeFunction *g_functions = NULL;

static int execute_statements(const ASTNode *statements,
                              RuntimeVariable **variables,
                              ExecResult *result);

static RuntimeFunction *find_function(RuntimeFunction *functions,
                                      const char *name) {
  for (RuntimeFunction *function = functions; function != NULL;
       function = function->next) {
    if (strcmp(function->name, name) == 0) {
      return function;
    }
  }
  return NULL;
}

static int register_functions(const ASTNode *program,
                               RuntimeFunction **functions) {
  for (const ASTNode *statement = program; statement != NULL;
       statement = statement->next) {
    if (statement->type != FUNC_DECL) {
      continue;
    }
    RuntimeFunction *function = malloc(sizeof(*function));
    if (function == NULL) {
      fprintf(stderr, "Out of memory while registering function\n");
      return 0;
    }
    function->name = statement->data.funcDeclaration.name;
    function->params = statement->data.funcDeclaration.params;
    function->body = statement->data.funcDeclaration.body;
    function->returnType = statement->data.funcDeclaration.returnType;
    function->next = *functions;
    *functions = function;
  }
  return 1;
}

static void free_functions(RuntimeFunction *functions) {
  while (functions != NULL) {
    RuntimeFunction *next = functions->next;
    free(functions);
    functions = next;
  }
}

static RuntimeVariable *find_variable(RuntimeVariable *variables,
                                      const char *name) {
  for (RuntimeVariable *variable = variables; variable != NULL;
       variable = variable->next) {
    if (strcmp(variable->name, name) == 0) {
      return variable;
    }
  }
  return NULL;
}

static int convert_value(RuntimeValue input, DataType target,
                         RuntimeValue *output);

static int evaluate(const ASTNode *expression, RuntimeVariable *variables,
                    RuntimeValue *result) {
  switch (expression->type) {
  case INT_LITERAL:
    result->type = TYPE_INT;
    result->data.int_value = expression->data.intValue;
    return 1;
  case FLOAT_LITERAL:
    result->type = TYPE_FLOAT;
    result->data.float_value = expression->data.floatValue;
    return 1;
  case BOOL_LITERAL:
    result->type = TYPE_BOOL;
    result->data.bool_value = expression->data.boolValue;
    return 1;
  case CHAR_LITERAL:
    result->type = TYPE_CHAR;
    result->data.char_value = expression->data.charValue;
    return 1;
  case STR_LITERAL:
    result->type = TYPE_STRING;
    result->data.string_value = expression->data.strValue;
    return 1;
  case IDENTIFIER_REF: {
    RuntimeVariable *variable =
        find_variable(variables, expression->data.identifier);
    if (variable == NULL || !variable->initialized) {
      fprintf(stderr, "Variable '%s' has no runtime value\n",
              expression->data.identifier);
      return 0;
    }
    *result = variable->value;
    return 1;
  }
  case BINARY_OP: {
    RuntimeValue left;
    RuntimeValue right;
    if (!evaluate(expression->data.binary.left, variables, &left) ||
        !evaluate(expression->data.binary.right, variables, &right)) {
      return 0;
    }

    if (expression->expressionType == TYPE_FLOAT) {
      float left_value = left.type == TYPE_INT ? (float)left.data.int_value
                                               : left.data.float_value;
      float right_value = right.type == TYPE_INT ? (float)right.data.int_value
                                                  : right.data.float_value;
      if (expression->data.binary.op == OP_DIV && right_value == 0.0f) {
        fprintf(stderr, "Division by zero\n");
        return 0;
      }
      result->type = TYPE_FLOAT;
      switch (expression->data.binary.op) {
      case OP_ADD: result->data.float_value = left_value + right_value; break;
      case OP_SUB: result->data.float_value = left_value - right_value; break;
      case OP_MUL: result->data.float_value = left_value * right_value; break;
      case OP_DIV: result->data.float_value = left_value / right_value; break;
      }
      return 1;
    }

    long long left_value = left.data.int_value;
    long long right_value = right.data.int_value;
    if (expression->data.binary.op == OP_DIV && right_value == 0) {
      fprintf(stderr, "Division by zero\n");
      return 0;
    }
    long long value;
    switch (expression->data.binary.op) {
    case OP_ADD: value = left_value + right_value; break;
    case OP_SUB: value = left_value - right_value; break;
    case OP_MUL: value = left_value * right_value; break;
    case OP_DIV: value = left_value / right_value; break;
    default: value = 0; break;
    }
    if (value < INT_MIN || value > INT_MAX) {
      fprintf(stderr, "Integer arithmetic overflow\n");
      return 0;
    }
    result->type = TYPE_INT;
    result->data.int_value = (int)value;
    return 1;
  }
  case FUNC_CALL: {
    RuntimeFunction *function =
        find_function(g_functions, expression->data.funcCall.name);
    if (function == NULL) {
      fprintf(stderr, "Undefined function: %s\n",
              expression->data.funcCall.name);
      return 0;
    }

    /* Bind each argument, evaluated in the caller's scope, to a fresh
     * parameter list that becomes the callee's entire variable scope: calls
     * do not see the caller's locals (no closures in this language). */
    RuntimeVariable *call_variables = NULL;
    const Parameter *param = function->params;
    const ASTNode *arg = expression->data.funcCall.args;
    int ok = 1;
    while (ok && param != NULL) {
      RuntimeValue arg_value;
      RuntimeVariable *binding = NULL;
      if (!evaluate(arg, variables, &arg_value)) {
        ok = 0;
        break;
      }
      binding = malloc(sizeof(*binding));
      if (binding == NULL || !convert_value(arg_value, param->type,
                                            &binding->value)) {
        fprintf(stderr, "Failed to bind argument '%s' for function '%s'\n",
                param->name, function->name);
        free(binding);
        ok = 0;
        break;
      }
      binding->name = param->name;
      binding->initialized = 1;
      binding->next = call_variables;
      call_variables = binding;
      param = param->next;
      arg = arg->next;
    }

    if (ok) {
      ExecResult call_result = {0};
      if (!execute_statements(function->body, &call_variables,
                              &call_result)) {
        ok = 0;
      } else if (call_result.returned) {
        *result = call_result.value;
      } else if (function->returnType != TYPE_VOID) {
        fprintf(stderr, "Function '%s' did not return a value\n",
                function->name);
        ok = 0;
      } else {
        result->type = TYPE_VOID;
      }
    }

    while (call_variables != NULL) {
      RuntimeVariable *next = call_variables->next;
      free(call_variables);
      call_variables = next;
    }
    return ok;
  }
  case VAR_DECL:
  case CONST_DECL:
  case FUNC_DECL:
  case PRINT_STMT:
  case RETURN_STMT:
    break;
  }
  fprintf(stderr, "Invalid expression node\n");
  return 0;
}

static int convert_value(RuntimeValue input, DataType target,
                         RuntimeValue *output) {
  output->type = target;
  switch (target) {
  case TYPE_INT:
    if (input.type == TYPE_INT) {
      output->data.int_value = input.data.int_value;
    } else if (input.type == TYPE_FLOAT &&
               (double)input.data.float_value >= (double)INT_MIN &&
               (double)input.data.float_value <= (double)INT_MAX) {
      output->data.int_value = (int)input.data.float_value;
    } else {
      return 0;
    }
    return 1;
  case TYPE_FLOAT:
    if (input.type == TYPE_FLOAT) {
      output->data.float_value = input.data.float_value;
    } else if (input.type == TYPE_INT) {
      output->data.float_value = (float)input.data.int_value;
    } else {
      return 0;
    }
    return 1;
  case TYPE_CHAR:
    if (input.type == TYPE_CHAR) {
      output->data.char_value = input.data.char_value;
    } else if (input.type == TYPE_INT && input.data.int_value >= CHAR_MIN &&
               input.data.int_value <= CHAR_MAX) {
      output->data.char_value = (char)input.data.int_value;
    } else {
      return 0;
    }
    return 1;
  case TYPE_BOOL:
    if (input.type != TYPE_BOOL) return 0;
    output->data.bool_value = input.data.bool_value;
    return 1;
  case TYPE_STRING:
    if (input.type != TYPE_STRING) return 0;
    output->data.string_value = input.data.string_value;
    return 1;
  case TYPE_VOID:
  case TYPE_UNKNOWN:
    return 0;
  }
  return 0;
}

static int execute_declaration(const ASTNode *declaration,
                               RuntimeVariable **variables) {
  const VarDeclaration *var = &declaration->data.varDeclaration;
  RuntimeVariable *variable = malloc(sizeof(*variable));
  if (variable == NULL) {
    fprintf(stderr, "Out of memory while creating runtime variable\n");
    return 0;
  }

  variable->name = var->identifier;
  variable->initialized = 0;
  if (var->init != NULL) {
    RuntimeValue value;
    DataType target = var->dataType != NULL ? *var->dataType : TYPE_UNKNOWN;
    if (!evaluate(var->init, *variables, &value) ||
        !convert_value(value, target, &variable->value)) {
      fprintf(stderr, "Initializer type does not match variable '%s'\n",
              var->identifier);
      free(variable);
      return 0;
    }
    variable->initialized = 1;
  }

  variable->next = *variables;
  *variables = variable;
  return 1;
}

static void print_value(RuntimeValue value) {
  switch (value.type) {
  case TYPE_INT: printf("%d\n", value.data.int_value); break;
  case TYPE_FLOAT: printf("%g\n", value.data.float_value); break;
  case TYPE_CHAR: printf("%c\n", value.data.char_value); break;
  case TYPE_BOOL: printf("%s\n", value.data.bool_value ? "true" : "false"); break;
  case TYPE_STRING: printf("%s\n", value.data.string_value); break;
  case TYPE_VOID: break;
  case TYPE_UNKNOWN: break;
  }
}

static int execute_print(const ASTNode *statement,
                         RuntimeVariable *variables) {
  RuntimeValue value;
  if (!evaluate(statement->data.print.value, variables, &value)) {
    return 0;
  }
  print_value(value);
  return 1;
}

static int execute_statements(const ASTNode *statements,
                              RuntimeVariable **variables,
                              ExecResult *result) {
  for (const ASTNode *statement = statements; statement != NULL;
       statement = statement->next) {
    switch (statement->type) {
    case VAR_DECL:
    case CONST_DECL:
      if (!execute_declaration(statement, variables)) {
        return 0;
      }
      break;
    case PRINT_STMT:
      if (!execute_print(statement, *variables)) {
        return 0;
      }
      break;
    case FUNC_DECL:
      /* Already registered by register_functions(); nothing to run here. */
      break;
    case FUNC_CALL: {
      RuntimeValue discarded;
      if (!evaluate(statement, *variables, &discarded)) {
        return 0;
      }
      break;
    }
    case RETURN_STMT:
      if (statement->data.returnStmt.value != NULL &&
          !evaluate(statement->data.returnStmt.value, *variables,
                    &result->value)) {
        return 0;
      }
      result->returned = 1;
      return 1;
    default:
      fprintf(stderr, "Unsupported statement\n");
      return 0;
    }
  }
  return 1;
}

int execute_program(const ASTNode *program) {
  RuntimeVariable *variables = NULL;
  int succeeded = register_functions(program, &g_functions);

  if (succeeded) {
    ExecResult result = {0};
    succeeded = execute_statements(program, &variables, &result);
  }

  while (variables != NULL) {
    RuntimeVariable *next = variables->next;
    free(variables);
    variables = next;
  }
  free_functions(g_functions);
  g_functions = NULL;
  return succeeded;
}
