# IFJ25 compiler

Compiler of the IFJ25 language (a subset of Wren) to IFJcode25, written in C11
for the IFJ/IAL 2025 project at FIT VUT. Symbol table variant: **vv-BVS**
(height-balanced binary search tree).

## Build and run

```sh
make                      # builds ./ifj25
./ifj25 < program.wren > program.code
./ifj25 --tokens < program.wren     # debug: print the token stream
```

Exit codes follow the assignment: 0 OK, 1 lexical, 2 syntax, 3–6 and 10
semantic errors, 99 internal error.

## Tests

```sh
make test
```

Runs `tests/run.sh`: lexer token dumps, compile-error exit codes, and complete
programs executed with `tests/ic25.py` (a local IFJcode25 interpreter; the
official `ic25int` on merlin is authoritative). Needs `python3`.

## Submission

```sh
make pack LOGIN=xlogin99  # creates xlogin99.tgz with a flat file list
```

Put `rozdeleni` and `dokumentace.pdf` into the repository root first; they are
included automatically. The Makefile builds the flat archive as well.

## Layout

```
include/    headers
src/        sources (main.c = entry point)
build/      object files (generated)
tests/      run.sh, ic25.py, lexer/ and programs/ test cases
examples/   example IFJ25 programs from the assignment
docs/       grammar + LL/precedence tables, lexer automaton, architecture, assignment
```

| Module | Purpose |
|---|---|
| `lexer` | finite automaton producing tokens (`docs/lexer-fsm.md`) |
| `parser`, `expr` | recursive descent + precedence analysis (`docs/grammar.md`) |
| `ast` | syntax tree |
| `symtable` | AVL tree symbol table |
| `semantic` | semantic checks, name resolution |
| `codegen` | IFJcode25 generation |
| `error`, `string_builder`, `vec`, `token` | utilities |

Design decisions: `docs/architecture.md`.
