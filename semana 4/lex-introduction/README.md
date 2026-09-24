# Práctica de Lex — Ejercicios 1 a 7

Analizador léxico construido con Flex a partir de `examples/lex-introduction/`.
Cada ejercicio agrega reglas nuevas a [`scanner.l`](scanner.l) sin tocar el archivo
generado `lex.yy.c`. Para reproducir cualquier prueba: copiar el archivo de
`pruebas/ejercicioN.txt` a `input.txt` y correr `make run`.

---

## Ejercicio 1 — Reconocer texto y mostrar su token

**Objetivo:** relacionar un patrón, su acción y `yytext`.

**Regla probada** (modificación temporal de la regla NUMBER, luego restaurada):

```lex
[0-9]+   { printf("Numero encontrado: %s\n", yytext); }
```

**Texto de prueba** (`pruebas/ejercicio1.txt`):

```
7 42 105
```

**Salida del programa:**

```
Numero encontrado: 7
Numero encontrado: 42
Numero encontrado: 105
```

**Explica: ¿qué contiene `yytext` cada vez que se ejecuta la acción?**

`yytext` es un puntero al lexema que la regla acaba de reconocer, no a toda la
entrada. Cada vez que se dispara la acción, contiene únicamente el texto de esa
coincidencia (primero `7`, luego `42`, luego `105`); Flex lo sobrescribe en cada
coincidencia nueva, así que solo es válido dentro de la acción que se está
ejecutando en ese momento.

Después de comprobarlo, la regla se restauró a `show_token("NUMBER", yytext)`
para mantener el mismo formato de salida en el resto de los ejercicios.

---

## Ejercicio 2 — Reconocer identificadores

**Objetivo:** entender los grupos de caracteres y las repeticiones.

**Regla usada** (ya existía en el archivo base):

```lex
[a-zA-Z_][a-zA-Z0-9_]*   { show_token("IDENTIFIER", yytext); }
```

**Texto de prueba** (`pruebas/ejercicio2.txt`):

```
name total2 _count x 2total
```

**Salida del programa:**

```
IDENTIFIER   name
IDENTIFIER   total2
IDENTIFIER   _count
IDENTIFIER   x
NUMBER       2
IDENTIFIER   total
```

**Explica: ¿por qué el analizador divide `2total` en lugar de reportarlo como un
único identificador inválido?**

El analizador léxico no "sabe" qué es un identificador inválido; solo intenta
hacer coincidir un patrón desde la posición actual del cursor. En `2total`, el
patrón de identificador no puede aplicar porque exige que el primer carácter
sea una letra o `_`, así que la regla que sí aplica en esa posición es
`[0-9]+`, que consume el `2`. Desde la nueva posición (después del `2`),
`total` sí empieza con una letra y forma un token IDENTIFIER completo. Decidir
que `2total` "debería" ser un solo nombre mal escrito es una regla semántica de
más alto nivel, no algo que el análisis léxico pueda deducir.

---

## Ejercicio 3 — Palabras reservadas y orden de las reglas

**Objetivo:** distinguir las palabras reservadas de los nombres comunes.

**Reglas agregadas** (antes de NUMBER e IDENTIFIER, después de LET):

```lex
"let"     { show_token("LET", yytext); }
"print"   { show_token("PRINT", yytext); }
"if"      { show_token("IF", yytext); }
```

**Texto de prueba** (`pruebas/ejercicio3.txt`):

```
print printer if if2 let letter
```

**Salida del programa (orden correcto — reservadas antes que IDENTIFIER):**

```
PRINT        print
IDENTIFIER   printer
IF           if
IDENTIFIER   if2
LET          let
IDENTIFIER   letter
```

**Experimento: mover la regla de IDENTIFIER antes que las palabras reservadas**

Al mover `[a-zA-Z_][a-zA-Z0-9_]*` al principio del archivo, `flex` advierte en
tiempo de compilación:

```
scanner.l: warning, rule cannot be matched
```

(una advertencia por cada regla de palabra reservada: LET, PRINT e IF), y la
entrada de prueba produce:

```
IDENTIFIER   print
IDENTIFIER   printer
IDENTIFIER   if
IDENTIFIER   if2
IDENTIFIER   let
IDENTIFIER   letter
```

Ninguna palabra reservada se reconoce como tal: el orden se restauró
(reservadas antes que IDENTIFIER) para el resto de la práctica.

**Explica: ¿por qué el orden importa para `print`, pero no hace que `printer`
se divida en dos tokens?**

Con `print`, las reglas PRINT e IDENTIFIER reconocen exactamente la misma
cantidad de caracteres (5), así que hay un empate de longitud; en un empate,
Flex elige la regla que aparece primero en el archivo, por eso el orden
importa. Con `printer` no hay empate: la regla IDENTIFIER llega a reconocer 7
caracteres mientras que PRINT solo llega a 5. Flex siempre prefiere primero la
coincidencia más larga posible, así que gana IDENTIFIER con el lexema completo
`printer` sin importar en qué orden estén escritas las reglas.

---

## Ejercicio 4 — Operadores y signos de puntuación

**Objetivo:** escribir reglas para símbolos literales.

**Reglas agregadas** (antes de la regla catch-all `.`):

```lex
"-"   { show_token("MINUS", yytext); }
"*"   { show_token("STAR", yytext); }
"("   { show_token("LPAREN", yytext); }
")"   { show_token("RPAREN", yytext); }
```

**Texto de prueba** (`pruebas/ejercicio4.txt`):

```
let result = (10 - 2) * 3;
```

**Salida del programa:**

```
LET          let
IDENTIFIER   result
ASSIGN       =
LPAREN       (
NUMBER       10
MINUS        -
NUMBER       2
RPAREN       )
STAR         *
NUMBER       3
SEMICOLON    ;
```

Once tokens, ninguno de tipo ERROR.

**Explica: ¿reconocer los paréntesis significa que el analizador verifica que
cada paréntesis de apertura tenga uno de cierre?**

No. El analizador léxico solo identifica que un carácter `(` o `)` forma un
token LPAREN o RPAREN; no sabe nada sobre si están balanceados o en qué
posición del programa aparecen. Verificar que cada apertura tenga su cierre
correspondiente (y en el orden correcto) es trabajo de una etapa posterior, el
análisis sintáctico, que sí conoce la estructura gramatical completa.

---

## Ejercicio 5 — Coincidencia más larga

**Objetivo:** observar cómo Lex elige entre patrones que pueden reconocer el
mismo inicio de texto.

**Reglas agregadas** (se conserva `"="` como ASSIGN):

```lex
"=="   { show_token("EQUAL_EQUAL", yytext); }
"="    { show_token("ASSIGN", yytext); }
">="   { show_token("GREATER_EQUAL", yytext); }
">"    { show_token("GREATER", yytext); }
```

**Texto de prueba** (`pruebas/ejercicio5.txt`):

```
= == > >= ===
```

**Salida del programa:**

```
ASSIGN       =
EQUAL_EQUAL  ==
GREATER      >
GREATER_EQUAL >=
EQUAL_EQUAL  ==
ASSIGN       =
```

**Explica: ¿por qué `===` se convierte en dos tokens con estas reglas?**

Lex siempre elige, en cada punto de la entrada, la coincidencia que abarca más
caracteres posible. Al llegar a `===`, la coincidencia más larga disponible es
`==` (dos caracteres); no existe una regla para `===` como tal, así que Lex
consume esos dos primeros signos como un token EQUAL_EQUAL. Queda un `=`
suelto, que se vuelve a analizar desde cero y coincide con la regla ASSIGN.
Por eso `===` se reporta como dos tokens: `==` y `=`.

---

## Ejercicio 6 — Números decimales

**Objetivo:** construir un patrón combinando partes pequeñas.

**Regla agregada** (se conserva NUMBER para los enteros):

```lex
[0-9]+"."[0-9]+   { show_token("DECIMAL", yytext); }
```

**Texto de prueba** (`pruebas/ejercicio6.txt`):

```
7 3.14 0.5 25.00
```

**Salida del programa:**

```
NUMBER       7
DECIMAL      3.14
DECIMAL      0.5
DECIMAL      25.00
```

**Prueba adicional** (`pruebas/ejercicio6b.txt`): `.5 5.`

**Salida del programa:**

```
ERROR        .
NUMBER       5
NUMBER       5
ERROR        .
```

Ninguna de las dos entradas cumple el patrón `[0-9]+"."[0-9]+` (falta el dígito
antes o después del punto), así que cada una produce un ERROR por el punto y un
NUMBER por el dígito, en el orden en que aparecen.

**Explica: ¿cuál es la diferencia entre `.` y `"."` en un patrón de Lex?**

Sin comillas, `.` es un metacarácter de expresión regular que significa
"cualquier carácter" (excepto salto de línea). Entre comillas, `"."` pierde su
significado especial y Lex lo trata como el carácter punto literal. Por eso el
patrón del decimal usa `"."`: para exigir exactamente un punto decimal, no
cualquier carácter.

---

## Ejercicio 7 — Ignorar espacios y comentarios

**Objetivo:** reconocer texto sin generar un token.

**Regla agregada** (acción vacía, antes de la regla catch-all; se conserva la
regla de espacios en blanco tal cual):

```lex
"//"[^\n]*   { /* Ignore comments. */ }
```

**Texto de prueba** (`pruebas/ejercicio7.txt`):

```
let x = 1; // Primer valor

    let y = 2; // Segundo valor
```

**Salida del programa:**

```
LET          let
IDENTIFIER   x
ASSIGN       =
NUMBER       1
SEMICOLON    ;
LET          let
IDENTIFIER   y
ASSIGN       =
NUMBER       2
SEMICOLON    ;
```

Diez tokens en total, correspondientes solo a las dos instrucciones `let`. Los
espacios, la línea vacía y los dos comentarios no generan ningún token.

**Explica: ¿por qué el patrón del comentario debe detenerse al llegar al salto
de línea?**

Un comentario de línea (`//...`) solo debe cubrir el resto de esa misma línea.
Al escribir el patrón como `"//"[^\n]*`, la clase `[^\n]` excluye
explícitamente el salto de línea, así que el comentario deja de consumir texto
justo antes del `\n`. Ese salto de línea queda disponible para que lo consuma
la regla de espacios en blanco `[ \t\r\n]+`, en lugar de ser "tragado" por el
comentario. Si el patrón incluyera `\n`, el comentario se fusionaría con el
contenido de la siguiente línea (perdiendo sus tokens) y, más adelante en la
práctica, rompería el conteo de línea del ejercicio 9, porque ese salto de
línea nunca llegaría a la regla que incrementa `current_line`.

---

## Estado final de `scanner.l` (acumulado hasta el ejercicio 7)

```lex
%option noyywrap
%{
#include <stdio.h>

void show_token(const char *kind, const char *text);
int lexical_errors = 0;
%}

%%
"let"                       { show_token("LET", yytext); }
"print"                     { show_token("PRINT", yytext); }
"if"                        { show_token("IF", yytext); }
[0-9]+"."[0-9]+             { show_token("DECIMAL", yytext); }
[0-9]+                      { show_token("NUMBER", yytext); }
[a-zA-Z_][a-zA-Z0-9_]*       { show_token("IDENTIFIER", yytext); }
"=="                        { show_token("EQUAL_EQUAL", yytext); }
"="                         { show_token("ASSIGN", yytext); }
">="                        { show_token("GREATER_EQUAL", yytext); }
">"                         { show_token("GREATER", yytext); }
"+"                         { show_token("PLUS", yytext); }
"-"                         { show_token("MINUS", yytext); }
"*"                         { show_token("STAR", yytext); }
"("                         { show_token("LPAREN", yytext); }
")"                         { show_token("RPAREN", yytext); }
";"                         { show_token("SEMICOLON", yytext); }
"//"[^\n]*                  { /* Ignore comments. */ }
[ \t\r\n]+                  { /* Ignore whitespace. */ }
.                           { show_token("ERROR", yytext); lexical_errors++; }
%%
```
