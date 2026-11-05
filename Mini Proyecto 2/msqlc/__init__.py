"""msqlc: compilador del pseudolenguaje MSQL a PostgreSQL, construido con Lark."""
from .compiler import compile_source, parse, CompileError

__all__ = ["compile_source", "parse", "CompileError"]
__version__ = "1.0.0"
