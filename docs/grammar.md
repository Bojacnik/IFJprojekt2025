# IFJ25 – LL(1) grammar, LL table, precedence table

Terminals are the token kinds from `src/token.h`. In this document:
`id` = `IDENT`, `gid` = `GID` (global `__x`), `int`/`float`/`string` = literals,
`null` = `KW_NULL`, `Ifj` = `KW_IFJ`, `EOL` = newline token, `eof` = end of input.

## 1. How the grammar stays LL(1)

1. **Optional newlines are handled by the lexer, not the grammar.**
   The lexer does not emit `EOL` at the start of the file, after another `EOL`,
   or after `(` `.` `,` an operator, `is` or `=` (the places where the spec allows
   a line break). A `//` comment counts as a newline, a `/* */` comment as a space.
   Every remaining `EOL` is significant, so the grammar only contains mandatory ones.
2. **Common prefixes are left-factored**: `static id` → `FUNC_KIND`,
   `id` at the start of a statement → `ID_TAIL`.
3. **The expression parser can take over an already-read `id`.**
   After `=`, `RHS → id RHS_ID` reads the identifier. The next token decides:
   `(` means a function call (rule 31), anything else means an expression
   whose first operand is that `id` (rule 32). The decision needs one token of lookahead.

Expressions (`EXPR`, `EXPR⁻`, `EXPR_REST`) are parsed by the precedence parser (section 4).

## 2. Grammar

```
 1 PROGRAM      → import string for Ifj EOL class id { EOL FUNCS } OPT_EOL eof
 2 OPT_EOL      → EOL
 3 OPT_EOL      → ε
 4 FUNCS        → static id FUNC_KIND EOL FUNCS
 5 FUNCS        → ε
 6 FUNC_KIND    → ( PARAMS ) BLOCK              -- function
 7 FUNC_KIND    → BLOCK                         -- static getter
 8 FUNC_KIND    → = ( id ) BLOCK                -- static setter
 9 PARAMS       → id PARAMS_TAIL
10 PARAMS       → ε
11 PARAMS_TAIL  → , id PARAMS_TAIL
12 PARAMS_TAIL  → ε
13 BLOCK        → { EOL STMTS }
14 STMTS        → STMT EOL STMTS
15 STMTS        → ε
16 STMT         → var id VAR_INIT
17 STMT         → id ID_TAIL
18 STMT         → gid = RHS
19 STMT         → BUILTIN_CALL
20 STMT         → if ( EXPR ) BLOCK ELSE_PART
21 STMT         → while ( EXPR ) BLOCK
22 STMT         → return RET_VAL
23 STMT         → BLOCK
24 VAR_INIT     → = RHS                         -- extension EXTSTAT only
25 VAR_INIT     → ε
26 ID_TAIL      → = RHS                         -- assignment or setter call
27 ID_TAIL      → ( ARGS )                      -- call as a statement
28 RHS          → id RHS_ID
29 RHS          → BUILTIN_CALL
30 RHS          → EXPR⁻                         -- expression not starting with id
31 RHS_ID       → ( ARGS )                      -- function call
32 RHS_ID       → EXPR_REST                     -- rest of an expression starting with id (may be empty)
33 BUILTIN_CALL → Ifj . id ( ARGS )
34 RET_VAL      → RHS
35 RET_VAL      → ε
36 ARGS         → TERM ARGS_TAIL
37 ARGS         → ε
38 ARGS_TAIL    → , TERM ARGS_TAIL
39 ARGS_TAIL    → ε
40 TERM         → id
41 TERM         → gid
42 TERM         → int
43 TERM         → float
44 TERM         → string
45 TERM         → null
46 ELSE_PART    → else BLOCK                     -- else is optional: ex0 omits it
47 ELSE_PART    → ε
```

Notes:
- `class id`: the identifier must be `Program`, checked by the parser (error 2).
- `import string`: the string must be `"ifj25"`, checked by the parser (error 2).
- A newline between `)` / `}` / `else` and the following block is an `EOL` token
  that no rule accepts, i.e. a syntax error, as the spec requires.
- `RET_VAL → ε` is needed by `ex0-vsechny-konstrukce.wren` (bare `return`);
  `RET_VAL → RHS` allows `return foo(x)` from the spec's own example.

## 3. FIRST and FOLLOW

Operators: `ops` = `* / + - < > <= >= == != is`.
Expression start without `id`: `exprstart⁻` = `gid int float string null (`.

| Nonterminal | FIRST | FOLLOW |
|---|---|---|
| PROGRAM | `import` | `$` |
| OPT_EOL | `EOL` ε | `eof` |
| FUNCS | `static` ε | `}` |
| FUNC_KIND | `(` `{` `=` | `EOL` |
| PARAMS | `id` ε | `)` |
| PARAMS_TAIL | `,` ε | `)` |
| BLOCK | `{` | `EOL` `else` |
| STMTS | `var id gid Ifj if while return {` ε | `}` |
| STMT | `var id gid Ifj if while return {` | `EOL` |
| VAR_INIT | `=` ε | `EOL` |
| ID_TAIL | `=` `(` | `EOL` |
| RHS | `id Ifj` exprstart⁻ | `EOL` |
| RHS_ID | `(` ops ε | `EOL` |
| BUILTIN_CALL | `Ifj` | `EOL` |
| RET_VAL | `id Ifj` exprstart⁻ ε | `EOL` |
| ARGS | `id gid int float string null` ε | `)` |
| ARGS_TAIL | `,` ε | `)` |
| TERM | `id gid int float string null` | `,` `)` |
| ELSE_PART | `else` ε | `EOL` |

## 4. LL table

Empty cells are syntax errors (exit code 2). No cell contains two rules,
so the grammar is LL(1).

| Nonterminal | lookahead → rule |
|---|---|
| PROGRAM | `import`→1 |
| OPT_EOL | `EOL`→2, `eof`→3 |
| FUNCS | `static`→4, `}`→5 |
| FUNC_KIND | `(`→6, `{`→7, `=`→8 |
| PARAMS | `id`→9, `)`→10 |
| PARAMS_TAIL | `,`→11, `)`→12 |
| BLOCK | `{`→13 |
| STMTS | `var` `id` `gid` `Ifj` `if` `while` `return` `{`→14, `}`→15 |
| STMT | `var`→16, `id`→17, `gid`→18, `Ifj`→19, `if`→20, `while`→21, `return`→22, `{`→23 |
| VAR_INIT | `=`→24, `EOL`→25 |
| ID_TAIL | `=`→26, `(`→27 |
| RHS | `id`→28, `Ifj`→29, `gid` `int` `float` `string` `null` `(`→30 |
| RHS_ID | `(`→31, ops `EOL`→32 |
| BUILTIN_CALL | `Ifj`→33 |
| RET_VAL | `id` `Ifj` `gid` `int` `float` `string` `null` `(`→34, `EOL`→35 |
| ARGS | `id` `gid` `int` `float` `string` `null`→36, `)`→37 |
| ARGS_TAIL | `,`→38, `)`→39 |
| TERM | `id`→40, `gid`→41, `int`→42, `float`→43, `string`→44, `null`→45 |
| ELSE_PART | `else`→46, `EOL`→47 |

In the recursive-descent parser every row is one function and every cell one
`case` of `switch (p->cur.kind)`.

## 5. Precedence table (expressions)

Operator groups (spec §5.1, all left-associative, top = highest priority):

| Group | Operators |
|---|---|
| M | `*` `/` |
| A | `+` `-` |
| R | `<` `>` `<=` `>=` |
| I | `is` |
| E | `==` `!=` |

`i` = term: `id`, `gid`, `int`, `float`, `string`, `null`, and the type names
`Num` `String` `Null` (valid only as the right operand of `is`).

Rows: topmost terminal on the stack. Columns: input token.
`<` shift with handle mark, `=` shift, `>` reduce, empty = syntax error.

| top \ input | M | A | R | I | E | ( | ) | i | $ |
|---|---|---|---|---|---|---|---|---|---|
| **M** | > | > | > | > | > | < | > | < | > |
| **A** | < | > | > | > | > | < | > | < | > |
| **R** | < | < | > | > | > | < | > | < | > |
| **I** | < | < | < | > | > | < | > | < | > |
| **E** | < | < | < | < | > | < | > | < | > |
| **(** | < | < | < | < | < | < | = | < | |
| **)** | > | > | > | > | > | | > | | > |
| **i** | > | > | > | > | > | | > | | > |
| **$** | < | < | < | < | < | < | | < | accept |

**End of an expression (`$`):** any token that cannot be part of an expression
(`EOL`, `{`, `,`, …), or `)` when the parser's own parenthesis depth is 0
(this is how `if ( EXPR )` and `while ( EXPR )` end).

**Reduction rules:**

```
E → i
E → ( E )
E → E * E | E / E
E → E + E | E - E
E → E < E | E > E | E <= E | E >= E
E → E is T        -- T must be Num, String or Null, otherwise error 2
E → E == E | E != E
```

**Seeded start (rule 32):** when called from `RHS_ID`, the parser starts with
`$ i` already on the stack (the `id` read by the LL parser) instead of `$`.
