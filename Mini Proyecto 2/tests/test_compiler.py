"""Pruebas del compilador MSQL -> PostgreSQL.  Ejecutar con:  python -m pytest"""
from pathlib import Path

import pytest

from msqlc import CompileError, compile_source

ROOT = Path(__file__).resolve().parent.parent
EXAMPLES = sorted((ROOT / "examples").glob("*.msql"))


# ---------- Ejemplos de la especificación (sección 5) ----------
def test_spec_ejemplo1_make():
    assert compile_source("MAKE users (id id, name text, email text!, created time)") == (
        "CREATE TABLE users (\n"
        "  id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,\n"
        "  name TEXT,\n"
        "  email TEXT NOT NULL,\n"
        "  created TIMESTAMPTZ DEFAULT NOW()\n"
        ");\n"
    )


def test_spec_ejemplo2_insert():
    assert compile_source("PUT users (name = 'Alex', email = 'alex@test.com')") == (
        "INSERT INTO users (name, email)\nVALUES ('Alex', 'alex@test.com');\n"
    )


def test_spec_ejemplo3_select():
    assert compile_source("GET name, email FROM users WHERE name = 'Alex' SORT created TOP 5") == (
        "SELECT name, email\nFROM users\nWHERE name = 'Alex'\nORDER BY created\nLIMIT 5;\n"
    )


def test_spec_ejemplo4_update():
    assert compile_source("PUT users (name = 'Alexander') WHERE email = 'alex@test.com'") == (
        "UPDATE users\nSET name = 'Alexander'\nWHERE email = 'alex@test.com';\n"
    )


def test_spec_ejemplo5_delete_y_drop():
    assert compile_source("DROP users WHERE name = 'Alex'\nDROP users") == (
        "DELETE FROM users WHERE name = 'Alex';\n\nDROP TABLE users;\n"
    )


# ---------- Ejemplos adjuntos: cada .msql debe producir su .sql esperado ----------
@pytest.mark.parametrize("src", EXAMPLES, ids=lambda p: p.name)
def test_ejemplos_adjuntos(src):
    expected = ROOT / "output" / (src.stem + ".sql")
    assert compile_source(src.read_text(encoding="utf-8")) == expected.read_text(encoding="utf-8")


# ---------- Reglas del lenguaje ----------
def test_palabras_clave_sin_distincion_de_mayusculas():
    assert compile_source("get ALL from t sort a desc top 1") == compile_source("GET all FROM t SORT a DESC TOP 1")


def test_bang_agrega_not_null_a_cualquier_tipo():
    sql = compile_source("MAKE t (a bool!, b time!, c num!)")
    assert "a BOOLEAN NOT NULL DEFAULT FALSE" in sql
    assert "b TIMESTAMPTZ NOT NULL DEFAULT NOW()" in sql
    assert "c NUMERIC NOT NULL" in sql


def test_comillas_dobles_se_convierten_en_cadena():
    assert "email <> 'x@y.com'" in compile_source('GET id FROM u WHERE email != "x@y.com"')


def test_escape_de_comilla_simple():
    assert "VALUES ('O''Brien')" in compile_source("PUT u (name = 'O''Brien')")


def test_null_se_traduce_a_is_null():
    sql = compile_source("GET a FROM t WHERE b = null OR c != NULL")
    assert "WHERE b IS NULL OR c IS NOT NULL" in sql


def test_not_y_parentesis():
    sql = compile_source("GET a FROM t WHERE NOT (a = 1 OR a = 2) AND b > -3.5")
    assert "WHERE NOT (a = 1 OR a = 2) AND b > -3.5" in sql


def test_comentarios_y_lineas_vacias():
    src = "# esquema\n\nMAKE t (id id)   -- tabla\n\n   \n-- fin\nDROP t\n"
    assert compile_source(src) == (
        "CREATE TABLE t (\n  id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY\n);\n\nDROP TABLE t;\n"
    )


def test_identificador_que_empieza_como_palabra_clave():
    assert compile_source("GET topic FROM makers") == "SELECT topic\nFROM makers;\n"


def test_archivo_vacio():
    assert compile_source("") == ""


# ---------- Errores ----------
@pytest.mark.parametrize("src, fragmento", [
    ("MAKE t (a txt)", "token inesperado 'txt'"),
    ("GET a FROM", "termina antes de tiempo"),
    ("MAKE t (a text", "se esperaba"),
    ("GET * FROM t", "carácter inesperado '*'"),
    ("MAKE t (a text, a num)", "columna duplicada"),
    ("MAKE t (a id, b id)", "más de una clave primaria"),
    ("PUT t (a = 1, a = 2)", "más de una vez"),
    ("GET a FROM t TOP 0", "mayor que cero"),
    ("MAKE t (a text) DROP t", "token inesperado"),
])
def test_errores(src, fragmento):
    with pytest.raises(CompileError) as e:
        compile_source(src)
    assert fragmento in str(e.value)


def test_error_reporta_linea_correcta():
    with pytest.raises(CompileError) as e:
        compile_source("MAKE t (id id)\nGET a FROM t\nPUT t (a = )")
    assert e.value.line == 3
