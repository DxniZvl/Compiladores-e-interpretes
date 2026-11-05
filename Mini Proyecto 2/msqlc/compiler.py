"""
Compilador MSQL -> PostgreSQL construido con Lark.

Fases:
  1. Análisis léxico y sintáctico  (Lark, parser LALR(1) + lexer contextual)
  2. Construcción del AST          (Transformer -> dataclasses de ast_nodes)
  3. Análisis semántico            (validaciones por instrucción)
  4. Generación de código          (AST -> texto SQL de PostgreSQL)
"""
from __future__ import annotations

from pathlib import Path

from lark import Lark, Transformer, Token, v_args
from lark.exceptions import UnexpectedInput, UnexpectedCharacters, UnexpectedToken, UnexpectedEOF

from . import ast_nodes as A

GRAMMAR_PATH = Path(__file__).with_name("grammar.lark")

# Mapeo de tipos del pseudolenguaje -> PostgreSQL (sección 3 de la especificación)
TYPE_MAP = {
    "id":   ("BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY", None),
    "num":  ("NUMERIC", None),
    "text": ("TEXT", None),
    "bool": ("BOOLEAN", "DEFAULT FALSE"),
    "time": ("TIMESTAMPTZ", "DEFAULT NOW()"),
}

RESERVED = {
    "make", "get", "put", "drop", "from", "where", "sort", "top", "all",
    "and", "or", "not", "asc", "desc", "true", "false", "null",
}


class CompileError(Exception):
    """Error de compilación con número de línea y columna."""

    def __init__(self, message: str, line: int | None = None, column: int | None = None,
                 source_line: str | None = None):
        self.message = message
        self.line = line
        self.column = column
        self.source_line = source_line
        super().__init__(self.__str__())

    def __str__(self) -> str:
        where = f"línea {self.line}" if self.line else "?"
        if self.line and self.column:
            where += f", columna {self.column}"
        text = f"[{where}] {self.message}"
        if self.source_line is not None and self.column:
            text += f"\n    {self.source_line}\n    {' ' * (self.column - 1)}^"
        return text


# ---------------------------------------------------------------------------
# 1. Parser
# ---------------------------------------------------------------------------
_parser: Lark | None = None


def get_parser() -> Lark:
    global _parser
    if _parser is None:
        _parser = Lark(
            GRAMMAR_PATH.read_text(encoding="utf-8"),
            parser="lalr",
            lexer="contextual",
            propagate_positions=True,
            maybe_placeholders=False,
        )
    return _parser


# ---------------------------------------------------------------------------
# 2. Árbol de Lark -> AST propio
# ---------------------------------------------------------------------------
@v_args(inline=True)
class ToAST(Transformer):
    def start(self, *stmts):
        return list(stmts)

    # ---- MAKE ----
    @v_args(meta=True)
    def make_stmt(self, meta, children):
        name, *cols = children
        return A.Make(table=str(name), columns=list(cols), line=meta.line)

    def column_def(self, name, type_spec):
        base, not_null = type_spec
        return A.Column(name=str(name), type=base, not_null=not_null,
                        line=name.line, column=name.column)

    def type_spec(self, type_tok, bang=None):
        return (str(type_tok).lower(), bang is not None)

    # ---- GET ----
    @v_args(meta=True)
    def get_stmt(self, meta, children):
        fields, table, *clauses = children
        stmt = A.Get(fields=fields, table=str(table), line=meta.line)
        for c in clauses:
            if isinstance(c, A.Where):
                stmt.where = c.condition
            elif isinstance(c, A.Sort):
                stmt.sort = c
            elif isinstance(c, A.Top):
                stmt.top = c.n
        return stmt

    def all_fields(self):
        return None  # None == todas las columnas (*)

    def named_fields(self, *names):
        return [str(n) for n in names]

    def sort_clause(self, field, direction=None):
        return A.Sort(field=str(field), direction=str(direction).upper() if direction else None)

    def top_clause(self, n):
        return A.Top(int(n))

    # ---- PUT ----
    @v_args(meta=True)
    def put_stmt(self, meta, children):
        table, *rest = children
        assignments = [r for r in rest if isinstance(r, A.Assignment)]
        where = next((r.condition for r in rest if isinstance(r, A.Where)), None)
        return A.Put(table=str(table), assignments=assignments, where=where, line=meta.line)

    def assignment(self, name, value):
        return A.Assignment(field=str(name), value=value, line=name.line, column=name.column)

    # ---- DROP ----
    @v_args(meta=True)
    def drop_stmt(self, meta, children):
        table, where = (children + [None])[:2]
        return A.Drop(table=str(table), where=where.condition if where else None, line=meta.line)

    # ---- Condiciones ----
    def where_clause(self, cond):
        return A.Where(cond)

    def or_expr(self, *parts):
        return A.BoolOp("OR", list(parts))

    def and_expr(self, *parts):
        return A.BoolOp("AND", list(parts))

    def not_cond(self, cond):
        return A.Not(cond)

    def group(self, cond):
        return A.Group(cond)

    def comparison(self, name, op, value):
        op = str(op)
        if op == "!=":
            op = "<>"  # forma estándar de SQL
        return A.Comparison(field=str(name), op=op, value=value)

    # ---- Literales ----
    def string(self, tok: Token):
        raw = str(tok)
        if raw[0] == "'":
            text = raw[1:-1].replace("''", "'")
        else:  # comillas dobles -> se interpretan como cadena, no como identificador
            text = bytes(raw[1:-1], "utf-8").decode("unicode_escape") if "\\" in raw else raw[1:-1]
        return A.Literal(kind="string", value=text)

    def number(self, tok):
        return A.Literal(kind="number", value=str(tok))

    def true(self, _tok):
        return A.Literal(kind="bool", value=True)

    def false(self, _tok):
        return A.Literal(kind="bool", value=False)

    def null(self, _tok):
        return A.Literal(kind="null", value=None)


# ---------------------------------------------------------------------------
# 3. Análisis semántico
# ---------------------------------------------------------------------------
def check(stmt: A.Statement, lines: list[str]) -> None:
    def err(msg, line=None, column=None):
        line = line or stmt.line
        src = lines[line - 1] if line and line <= len(lines) else None
        raise CompileError(msg, line, column, src)

    for ident in _identifiers(stmt):
        if ident.lower() in RESERVED:
            err(f"'{ident}' es una palabra reservada y no puede usarse como identificador")

    if isinstance(stmt, A.Make):
        seen = set()
        ids = 0
        for col in stmt.columns:
            key = col.name.lower()
            if key in seen:
                err(f"columna duplicada '{col.name}' en la tabla '{stmt.table}'", col.line, col.column)
            seen.add(key)
            if col.type == "id":
                ids += 1
        if ids > 1:
            err(f"la tabla '{stmt.table}' define más de una clave primaria de tipo 'id'")

    elif isinstance(stmt, A.Put):
        seen = set()
        for a in stmt.assignments:
            key = a.field.lower()
            if key in seen:
                err(f"el campo '{a.field}' se asigna más de una vez", a.line, a.column)
            seen.add(key)

    elif isinstance(stmt, A.Get):
        if stmt.top is not None and stmt.top <= 0:
            err("TOP debe ser un entero mayor que cero")


def _identifiers(stmt):
    yield stmt.table
    if isinstance(stmt, A.Make):
        yield from (c.name for c in stmt.columns)
    elif isinstance(stmt, A.Get):
        yield from (stmt.fields or [])
        if stmt.sort:
            yield stmt.sort.field


# ---------------------------------------------------------------------------
# 4. Generación de código PostgreSQL
# ---------------------------------------------------------------------------
INDENT = "  "


def gen_literal(lit: A.Literal) -> str:
    if lit.kind == "string":
        return "'" + lit.value.replace("'", "''") + "'"
    if lit.kind == "bool":
        return "TRUE" if lit.value else "FALSE"
    if lit.kind == "null":
        return "NULL"
    return lit.value


def gen_cond(c) -> str:
    if isinstance(c, A.Comparison):
        if c.value.kind == "null" and c.op in ("=", "<>"):
            # En SQL "x = NULL" nunca es verdadero; se traduce a IS [NOT] NULL
            return f"{c.field} IS {'NOT ' if c.op == '<>' else ''}NULL"
        return f"{c.field} {c.op} {gen_literal(c.value)}"
    if isinstance(c, A.BoolOp):
        return f" {c.op} ".join(gen_cond(p) for p in c.parts)
    if isinstance(c, A.Not):
        return f"NOT {gen_cond(c.operand)}"
    if isinstance(c, A.Group):
        return f"({gen_cond(c.inner)})"
    raise TypeError(c)


def gen_column(col: A.Column) -> str:
    sql_type, default = TYPE_MAP[col.type]
    parts = [col.name, sql_type]
    if col.not_null and col.type != "id":  # una PK ya es NOT NULL
        parts.append("NOT NULL")
    if default:
        parts.append(default)
    return " ".join(parts)


def generate(stmt: A.Statement) -> str:
    if isinstance(stmt, A.Make):
        cols = ",\n".join(INDENT + gen_column(c) for c in stmt.columns)
        return f"CREATE TABLE {stmt.table} (\n{cols}\n);"

    if isinstance(stmt, A.Get):
        fields = "*" if stmt.fields is None else ", ".join(stmt.fields)
        out = [f"SELECT {fields}", f"FROM {stmt.table}"]
        if stmt.where is not None:
            out.append(f"WHERE {gen_cond(stmt.where)}")
        if stmt.sort is not None:
            order = f"ORDER BY {stmt.sort.field}"
            if stmt.sort.direction:
                order += f" {stmt.sort.direction}"
            out.append(order)
        if stmt.top is not None:
            out.append(f"LIMIT {stmt.top}")
        return "\n".join(out) + ";"

    if isinstance(stmt, A.Put):
        if stmt.where is None:
            names = ", ".join(a.field for a in stmt.assignments)
            values = ", ".join(gen_literal(a.value) for a in stmt.assignments)
            return f"INSERT INTO {stmt.table} ({names})\nVALUES ({values});"
        sets = ", ".join(f"{a.field} = {gen_literal(a.value)}" for a in stmt.assignments)
        return f"UPDATE {stmt.table}\nSET {sets}\nWHERE {gen_cond(stmt.where)};"

    if isinstance(stmt, A.Drop):
        if stmt.where is None:
            return f"DROP TABLE {stmt.table};"
        return f"DELETE FROM {stmt.table} WHERE {gen_cond(stmt.where)};"

    raise TypeError(stmt)


# ---------------------------------------------------------------------------
# API pública
# ---------------------------------------------------------------------------
def parse(source: str) -> list[A.Statement]:
    lines = source.splitlines()
    try:
        tree = get_parser().parse(source)
    except UnexpectedInput as e:
        raise _syntax_error(e, lines) from None
    stmts = ToAST().transform(tree)
    for s in stmts:
        check(s, lines)
    return stmts


def compile_source(source: str) -> str:
    """Compila un programa MSQL completo y devuelve el SQL de PostgreSQL."""
    stmts = parse(source)
    return "\n\n".join(generate(s) for s in stmts) + ("\n" if stmts else "")


def _syntax_error(e: UnexpectedInput, lines: list[str]) -> CompileError:
    line, col = getattr(e, "line", None), getattr(e, "column", None)
    src = lines[line - 1] if line and 0 < line <= len(lines) else None
    if isinstance(e, UnexpectedCharacters):
        msg = f"carácter inesperado '{e.char}'"
    elif isinstance(e, UnexpectedEOF):
        msg = "fin de archivo inesperado; la instrucción está incompleta"
        if lines:
            line, col, src = len(lines), len(lines[-1]) + 1, lines[-1]
    elif isinstance(e, UnexpectedToken):
        tok = e.token
        if tok.type == "$END":
            msg = "la instrucción termina antes de tiempo"
            if lines:
                line = len(lines)
                col, src = len(lines[-1].rstrip()) + 1, lines[-1]
        elif tok.type == "_NL":
            msg = "la instrucción está incompleta al final de la línea"
            col = len(src.rstrip()) + 1 if src else col
        else:
            msg = f"token inesperado '{tok}'"
        expected = sorted(_pretty(t) for t in e.expected)
        if expected:
            msg += f"; se esperaba: {', '.join(expected)}"
    else:
        msg = str(e)
    return CompileError("error de sintaxis: " + msg, line, col, src)


_PRETTY = {
    "NAME": "identificador", "TYPE_NAME": "tipo (id, num, text, bool, time)",
    "LPAR": "'('", "RPAR": "')'", "COMMA": "','", "EQUAL": "'='", "BANG": "'!'",
    "STRING": "cadena", "SIGNED_NUMBER": "número", "INT": "entero",
    "COMP_OP": "operador de comparación", "_NL": "fin de línea", "$END": "fin de archivo",
    "DIRECTION": "ASC/DESC",
}


def _pretty(t: str) -> str:
    if t in _PRETTY:
        return _PRETTY[t]
    return t.lstrip("_").upper()
