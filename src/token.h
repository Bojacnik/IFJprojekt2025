/*
 * Implementace překladače imperativního jazyka IFJ25
 * Token kinds and the token structure.
 * Autoři: TODO (login, jméno)
 */

#ifndef IFJ25_TOKEN_H
#define IFJ25_TOKEN_H

#include <stddef.h>
#include <stdio.h>

// One entry per terminal of the grammar (see docs/grammar.md).
// The kind doubles as the column index of the LL table.
#define TOKEN_LIST(X)                                                          \
    /* special */                                                              \
    X(EOF) X(EOL)                                                              \
    /* tokens with a value */                                                  \
    X(IDENT) X(GID) X(INT) X(FLOAT) X(STRING)                                  \
    /* keywords */                                                             \
    X(KW_CLASS) X(KW_ELSE) X(KW_FOR) X(KW_IF) X(KW_IMPORT) X(KW_IS)            \
    X(KW_NULL) X(KW_RETURN) X(KW_STATIC) X(KW_VAR) X(KW_WHILE) X(KW_IFJ)       \
    X(KW_TYPE_NUM) X(KW_TYPE_STRING) X(KW_TYPE_NULL)                           \
    /* punctuation */                                                          \
    X(LPAREN) X(RPAREN) X(LBRACE) X(RBRACE) X(COMMA) X(DOT)                    \
    /* operators */                                                            \
    X(MUL) X(DIV) X(PLUS) X(MINUS) X(LT) X(GT) X(LE) X(GE) X(EQ) X(NE)         \
    X(ASSIGN)

typedef enum {
#define X(name) TK_##name,
    TOKEN_LIST(X)
#undef X
    TK_COUNT
} TokenKind;

extern const char *const TOKEN_NAMES[TK_COUNT];

typedef struct {
    TokenKind kind;
    int line, col;      // position of the first character
    size_t len;         // TK_STRING: length in bytes (the value may contain '\0')
    union {
        char *str;      // TK_IDENT, TK_GID, TK_STRING: heap, owned by the token
        long long i;    // TK_INT
        double f;       // TK_FLOAT
    };
} Token;

// Frees the token's heap payload (if any). Safe to call on any token.
void token_free(Token *t);

// Debug dump: "line:col KIND [value]\n".
void token_print(FILE *out, const Token *t);

#endif
