/*
 * Implementace překladače imperativního jazyka IFJ25
 * Expression parsing: operator precedence analysis (see docs/grammar.md, section 5).
 * Autoři: TODO (login, jméno)
 */

#ifndef IFJ25_EXPR_H
#define IFJ25_EXPR_H

#include "ast.h"
#include "parser.h"

// Parses an expression starting at the parser's current token.
// If seed != NULL, it is an operand the LL parser has already read (rule 32):
// the analysis starts as if it were the first input symbol.
// The expression ends at the first token that cannot continue it, or at a ')'
// without a matching '(' inside the expression; that token is not consumed.
Expr *expr_parse(Parser *p, Expr *seed);

// Builds the leaf for the current term token (identifier, literal, null or
// type name) and consumes it. The caller checks that the token is a term.
Expr *expr_leaf(Parser *p);

#endif
