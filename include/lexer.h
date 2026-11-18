/*
 * Implementace překladače imperativního jazyka IFJ25
 * Lexical analyser (finite automaton, see docs/lexer-fsm.md).
 * Autoři: TODO (login, jméno)
 */

#ifndef IFJ25_LEXER_H
#define IFJ25_LEXER_H

#include <stdbool.h>
#include <stdio.h>

#include "string_builder.h"
#include "token.h"

typedef struct {
    FILE *in;
    int cur;            // current (not yet consumed) character, or EOF
    int line, col;      // position of `cur`
    TokenKind last;     // kind of the last emitted token (for the EOL rule)
    bool has_last;      // false until the first token is emitted
    StringBuilder *buf; // lexeme buffer, reused for every token
} Lexer;

void lexer_init(Lexer *l, FILE *in);

void lexer_free(Lexer *l);

// Returns the next token. On a lexical error the compiler exits with code 1.
// The caller owns the token's payload (see token_free).
Token lexer_next(Lexer *l);

#endif
