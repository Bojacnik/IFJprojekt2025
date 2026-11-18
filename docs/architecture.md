# IFJ25 compiler – architecture and design decisions

```
stdin → lexer.c → parser.c (+ expr.c) → AST (ast.c) → semantic.c → codegen.c → stdout
```

Sources are in `src/`, the matching headers in `include/`.

| File | Part |
|---|---|
| `src/main.c` | entry point, `--tokens` debug dump |
| `src/lexer.c` | finite automaton (`docs/lexer-fsm.md`), EOL rule |
| `src/parser.c` | recursive descent, one function per LL table row (`docs/grammar.md`) |
| `src/expr.c` | operator precedence analysis (`docs/grammar.md` §5) |
| `src/ast.c` | syntax tree |
| `src/symtable.c` | symbol table – height-balanced (AVL) BST (variant vv-BVS) |
| `src/semantic.c` | 2-pass semantic analysis |
| `src/codegen.c` | IFJcode25 generation + runtime helper subroutines |
| `src/error.c` | exit codes, `fatal()`, allocation wrappers |
| `src/string_builder.c`, `src/vec.c` | dynamic string / pointer array |

## Error handling
The compiler stops at the first error: `fatal(code, line, col, msg)` prints to
stderr and exits with the code from the assignment. Allocation failure → 99.

## Semantic analysis
- **Pass 1** inserts all function headers into an AVL table (`f:name/arity`,
  `g:name` getter, `s:name` setter, `n:name` "a function of this name exists").
  Duplicates → 4, missing `main()` → 3. Functions may be used before their definition.
- **Pass 2** walks each body with a stack of AVL tables, one per block.
  Parameters share the scope of the function body's top level (as in Wren),
  so `var x` redefining parameter `x` → 4.
- Name resolution: `x` in an expression is a local variable, else a getter, else 3.
  Assignment target: local variable, else setter, else 3. `__x` is a global.
- Calls: unknown name → 3; known name with another arity → 5 (also builtins).
- Compile-time type checks where types are known from literals: wrong literal
  builtin argument → 5, incompatible operands (`"1" + 2`, `null + 1`, …) → 6.

## Code generation
- **Num is always an IFJcode25 float.** `Ifj.write` / `Ifj.str` print integral
  values as integers (`ISINT` + `FLOAT2INT`), others with `%a` / `%.2f`.
- **Expressions use the data stack**; every operator is a helper subroutine
  (`$$add`, `$$eq`, …) that checks operand types at run time (exit 26) and pushes
  the result. Builtins are helpers too (`$$b_length`, …; wrong type → exit 25).
  Helpers only use `GF@$$*` temporaries and never call user code.
- **Calling convention**: arguments are evaluated onto the data stack (so a
  getter used as an argument can create its own frame first), then
  `CREATEFRAME`, `DEFVAR TF@$p0..`, `POPS`, `CALL`. The callee `PUSHFRAME`s,
  defines **all** its locals at the start (a `DEFVAR` inside a loop would fail
  on the second iteration), and returns its value on the data stack.
- **Block scoping**: IFJcode25 frames have no blocks, so every local gets a
  unique frame name `name$N`; parameters are `$p0, $p1, …` (cannot clash,
  identifiers start with a letter).
- Globals are `GF@__name`, defined and set to `nil` before `main` runs,
  so reading an unassigned global gives `null`.

## Deviations / extensions (mention in the documentation)
- `else` is optional (the course example `ex0-vsechny-konstrukce.wren` omits it);
  grammar rules 46–47, still LL(1).
- `return` without a value returns `null` (also used by `ex0`).
- EXTSTAT: `var x = expr` is accepted (rules 24–25).
- `return f(x)` (a call after `return`) is accepted, as in the spec's example.

## Testing
`make test` runs `tests/run.sh`: lexer token dumps, compile-error exit codes,
and complete programs whose output is compared after running the generated
code with `tests/ic25.py` – a small IFJcode25 interpreter written from spec
§10 for local use only. **The official `ic25int` on merlin is authoritative**;
run the programs there (and compare with `wren-cli`) before submitting.
