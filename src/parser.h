/*
 * Implementace překladače imperativního jazyka IFJ25
 * Syntax analysis: recursive descent (LL(1), see docs/grammar.md).
 * Autoři: TODO (login, jméno)
 */

#ifndef IFJ25_PARSER_H
#define IFJ25_PARSER_H

#include "ast.h"
#include "lexer.h"

typedef struct {
    Lexer *lexer;
    Token cur; // lookahead token
} Parser;

// Parses the whole program. On a syntax error the compiler exits with code 2.
Program *parse_program(Lexer *lexer);

/* Shared with the expression parser (expr.c). */

// Moves to the next token (frees the payload of the current one).
void parser_next(Parser *p);

// Takes ownership of the current token's string payload.
char *parser_take_str(Parser *p);

_Noreturn void parser_error(const Parser *p, const char *expected);

#endif
