/*
 * Implementace překladače imperativního jazyka IFJ25
 * Token kinds and the token structure.
 * Autoři: TODO (login, jméno)
 */

#include "token.h"

#include <stdlib.h>

const char *const TOKEN_NAMES[TK_COUNT] = {
#define X(name) #name,
    TOKEN_LIST(X)
#undef X
};

void token_free(Token *t) {
    if (t->kind == TK_IDENT || t->kind == TK_GID || t->kind == TK_STRING) {
        free(t->str);
        t->str = NULL;
    }
}

static void print_escaped(FILE *out, const char *s, size_t len) {
    fputc('"', out);
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char) s[i];
        switch (c) {
            case '\n': fputs("\\n", out); break;
            case '\r': fputs("\\r", out); break;
            case '\t': fputs("\\t", out); break;
            case '"':  fputs("\\\"", out); break;
            case '\\': fputs("\\\\", out); break;
            default:
                if (c < 32 || c >= 127) fprintf(out, "\\x%02x", c);
                else fputc(c, out);
        }
    }
    fputc('"', out);
}

void token_print(FILE *out, const Token *t) {
    fprintf(out, "%d:%d %s", t->line, t->col, TOKEN_NAMES[t->kind]);
    switch (t->kind) {
        case TK_IDENT:
        case TK_GID:    fprintf(out, " %s", t->str); break;
        case TK_INT:    fprintf(out, " %lld", t->i); break;
        case TK_FLOAT:  fprintf(out, " %.17g", t->f); break;
        case TK_STRING: fputc(' ', out); print_escaped(out, t->str, t->len); break;
        default: break;
    }
    fputc('\n', out);
}
