# IFJ25 – lexer finite automaton

State names match `State` in `src/lexer.c`. A transition labelled with a
character consumes it. "other" means any character not covered by another
transition from that state; an accepting state reached by "other" emits its
token **without** consuming that character (the next token starts with it).
Any missing transition is a lexical error (exit code 1).

Character classes: `L` letter, `D` digit, `H` hex digit, `W` letter/digit/`_`.

```mermaid
stateDiagram-v2
    direction LR
    [*] --> START

    START --> START : space, tab, CR, newline (EOL rule)
    START --> IDENT : L
    IDENT --> IDENT : W
    START --> UNDERSCORE : _
    UNDERSCORE --> GID : _
    GID --> GID : W

    START --> ZERO : 0
    START --> INT : 1-9
    ZERO --> HEX_PREFIX : x
    HEX_PREFIX --> HEX : H
    HEX --> HEX : H
    INT --> INT : D
    ZERO --> FRAC_FIRST : .
    INT --> FRAC_FIRST : .
    FRAC_FIRST --> FRAC : D
    FRAC --> FRAC : D
    ZERO --> EXP_FIRST : e E
    INT --> EXP_FIRST : e E
    FRAC --> EXP_FIRST : e E
    EXP_FIRST --> EXP_SIGN : + -
    EXP_FIRST --> EXP : D
    EXP_SIGN --> EXP : D
    EXP --> EXP : D

    START --> QUOTE1 : "
    QUOTE1 --> QUOTE2 : "
    QUOTE1 --> STR : other (not consumed)
    STR --> STR : char > 31 except " and backslash
    STR --> STR_ESC : backslash
    STR_ESC --> STR : " n r t backslash
    STR_ESC --> STR_HEX1 : x
    STR_HEX1 --> STR_HEX2 : H
    STR_HEX2 --> STR : H
    QUOTE2 --> ML_STR : "
    ML_STR --> ML_STR : any except "
    ML_STR --> ML_Q1 : "
    ML_Q1 --> ML_Q2 : "
    ML_Q1 --> ML_STR : other (not consumed)
    ML_Q2 --> ML_STR : other (not consumed)

    START --> SLASH : /
    SLASH --> LINE_COMMENT : /
    LINE_COMMENT --> LINE_COMMENT : any except newline
    LINE_COMMENT --> START : newline (not consumed)
    SLASH --> BLOCK : * (depth = 1)
    BLOCK --> BLOCK : other
    BLOCK --> BLOCK_STAR : *
    BLOCK --> BLOCK_SLASH : /
    BLOCK_STAR --> BLOCK_STAR : *
    BLOCK_STAR --> BLOCK : / (depth > 1, depth--)
    BLOCK_STAR --> START : / (depth = 1)
    BLOCK_SLASH --> BLOCK : * (depth++)
    BLOCK_SLASH --> BLOCK_SLASH : /

    START --> LT : <
    START --> GT : >
    START --> ASSIGN : =
    START --> BANG : !
```

## Accepting states and their tokens

| State | On "other" emits |
|---|---|
| IDENT | keyword (table lookup) or `IDENT` |
| GID | `GID` |
| ZERO, INT, HEX | `INT` (error if "other" is a `W` character) |
| FRAC, EXP | `FLOAT` (error if "other" is a `W` character) |
| QUOTE2 | `STRING` (empty) |
| SLASH | `DIV` |
| LT / GT / ASSIGN | `LT` / `GT` / `ASSIGN`; with `=` consumed: `LE` / `GE` / `EQ` |
| BANG | only `!=` → `NE` |

Tokens emitted directly from START: `( ) { } , . * + -`, `EOL`, `EOF`.
`STR` emits `STRING` on a closing `"`; `ML_Q2` emits `STRING` on the third `"`
(after trimming the whitespace-only first/last line).

## Errors (exit code 1)

- `ZERO` followed by a digit (leading zeros), a number directly followed by a letter
- `FRAC_FIRST`, `EXP_FIRST`, `EXP_SIGN`, `HEX_PREFIX` without a digit
- newline, control character or end of file inside `"..."`
- unknown escape, `\x` without two hex digits
- end of file inside `"""..."""` or a block comment
- single `_` start, lone `!`, any other unexpected character
