"""Nodos del árbol de sintaxis abstracta (AST) del pseudolenguaje MSQL."""
from __future__ import annotations

from dataclasses import dataclass, field
from typing import Optional, Union


# ---------- Valores y condiciones ----------
@dataclass
class Literal:
    kind: str            # "string" | "number" | "bool" | "null"
    value: object


@dataclass
class Comparison:
    field: str
    op: str
    value: Literal


@dataclass
class BoolOp:
    op: str              # "AND" | "OR"
    parts: list


@dataclass
class Not:
    operand: object


@dataclass
class Group:
    inner: object


Condition = Union[Comparison, BoolOp, Not, Group]


# ---------- Cláusulas auxiliares ----------
@dataclass
class Where:
    condition: Condition


@dataclass
class Sort:
    field: str
    direction: Optional[str] = None   # "ASC" | "DESC" | None


@dataclass
class Top:
    n: int


@dataclass
class Column:
    name: str
    type: str            # id | num | text | bool | time
    not_null: bool = False
    line: int = 0
    column: int = 0


@dataclass
class Assignment:
    field: str
    value: Literal
    line: int = 0
    column: int = 0


# ---------- Instrucciones ----------
@dataclass
class Make:
    table: str
    columns: list
    line: int = 0


@dataclass
class Get:
    fields: Optional[list]           # None => todas las columnas
    table: str
    where: Optional[Condition] = None
    sort: Optional[Sort] = None
    top: Optional[int] = None
    line: int = 0


@dataclass
class Put:
    table: str
    assignments: list
    where: Optional[Condition] = None
    line: int = 0


@dataclass
class Drop:
    table: str
    where: Optional[Condition] = None
    line: int = 0


Statement = Union[Make, Get, Put, Drop]
