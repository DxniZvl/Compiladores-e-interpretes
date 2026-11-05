"""
Interfaz de línea de comandos.

Uso:
  python -m msqlc archivo.msql               -> genera archivo.sql junto al original
  python -m msqlc archivo.msql -o salida.sql -> genera salida.sql
  python -m msqlc carpeta/ -o salida/        -> compila todos los .msql de la carpeta
  python -m msqlc archivo.msql --stdout      -> imprime el SQL en pantalla
  python -m msqlc --tree archivo.msql        -> muestra el árbol sintáctico de Lark
"""
import argparse
import sys
from pathlib import Path

from .compiler import CompileError, compile_source, get_parser


def compile_file(src: Path, dst: Path | None, to_stdout: bool) -> bool:
    try:
        sql = compile_source(src.read_text(encoding="utf-8"))
    except CompileError as e:
        print(f"{src}: {e}", file=sys.stderr)
        return False
    if to_stdout:
        print(f"-- {src.name}")
        print(sql)
    else:
        dst = dst or src.with_suffix(".sql")
        dst.parent.mkdir(parents=True, exist_ok=True)
        dst.write_text(sql, encoding="utf-8")
        print(f"{src} -> {dst}")
    return True


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(prog="msqlc", description="Compilador MSQL -> PostgreSQL (Lark)")
    ap.add_argument("input", help="archivo .msql o carpeta con archivos .msql")
    ap.add_argument("-o", "--output", help="archivo o carpeta de salida")
    ap.add_argument("--stdout", action="store_true", help="imprimir el SQL en lugar de escribir archivos")
    ap.add_argument("--tree", action="store_true", help="mostrar el árbol de análisis de Lark")
    args = ap.parse_args(argv)

    src = Path(args.input)
    if args.tree:
        print(get_parser().parse(src.read_text(encoding="utf-8")).pretty())
        return 0

    if src.is_dir():
        files = sorted(src.glob("*.msql"))
        out_dir = Path(args.output) if args.output else src
        ok = all([compile_file(f, out_dir / (f.stem + ".sql"), args.stdout) for f in files])
    else:
        ok = compile_file(src, Path(args.output) if args.output else None, args.stdout)
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
