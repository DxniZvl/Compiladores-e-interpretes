# Arepascript

Compilador/intérprete de **Arepascript**, mi lenguaje de programación inventado
(modismos venezolanos), construido sobre la arquitectura vista en clase
(flex + bison + AST + tabla de símbolos).

## Entregables de este trabajo

| Entregable | Archivo(s) |
|---|---|
| Analizador léxico | `lexer.l` |
| Analizador sintáctico | `parser.y` (solo construye el AST, no valida tipos) |
| AST | `ast.h`, `ast.c` |
| Tabla AST | `print_ast_table()` en `ast.c` (vista tabular con ID, nodo, detalle, hijo izq/der) |
| Analizador semántico | `semantic_analyzer.h`, `semantic_analyzer.c` |
| Tabla de símbolos | `symbol_table.h`, `symbol_table.c` |
| Ejecución (runtime) | `runtime.c` |

### Por qué el parser y el analizador semántico están separados

`parser.y` solo reconoce la gramática y arma el árbol (AST). No consulta la
tabla de símbolos ni valida tipos. Después, `semantic_analyzer.c` recorre ese
árbol ya construido, llena la tabla de símbolos y valida: tipos, variables no
declaradas, declaraciones duplicadas y asignaciones a `pichirre`. Así las dos
fases del compilador quedan claramente separadas y se pueden explicar por
separado en la evaluación.

## Reglas del lenguaje

- Todo en minúscula y sin tildes.
- Cada instrucción termina en `;`.
- Comentarios de una línea con `//`.

### Declarar variables

```
vaina nombre: tipo;                 // sin valor inicial
vaina nombre: tipo = valor;         // con valor inicial
vaina nombre = valor;               // tipo inferido del valor
pichirre NOMBRE: tipo = valor;      // constante, siempre con valor
pichirre NOMBRE = valor;            // constante, tipo inferido
```

- `vaina`: variable que puede cambiar.
- `pichirre`: valor fijo, no se puede reasignar.

### Tipos

| Tipo | Guarda | Ejemplo |
|---|---|---|
| `ta_completo` | entero | `42`, `-7` |
| `quebrao` | decimal | `3.14` |
| `cuento` | texto | `"hola"` |
| `letra` | un caracter | `'A'` |
| `si_o_no` | booleano | `chevere` (true) / `nada_que_ver` (false) |

### Asignar y mostrar valores

```
edad = 20;
echale edad;        // imprime el valor de una expresion
```

### Operaciones

`+  -  *  /` entre `ta_completo`/`quebrao` (si uno es `quebrao`, el resultado
es `quebrao`, igual que `int + float` en C). Dividir entre cero es error en
tiempo de ejecución.

### Mensajes de error (analizador semántico)

| Mensaje | Situación |
|---|---|
| `Epa, chamo, eso no es <tipo>.` | El tipo no coincide |
| `Ese pichirre no suelta el valor.` | Intentas reasignar una constante |
| `¿Y esa vaina de dónde salió?` | Usas una variable no declarada |

## Compilar y ejecutar

```sh
make
./arepascript -d ejemplos/demo.arepa   # -d muestra tabla AST, AST, tabla de simbolos
./arepascript ejemplos/error_pichirre.arepa
./arepascript ejemplos/error_no_declarada.arepa
./arepascript ejemplos/error_tipo.arepa
```
