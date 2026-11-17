/*
 * Implementace překladače imperativního jazyka IFJ25
 * Entry point: reads IFJ25 source from stdin, writes IFJcode25 to stdout.
 * Autoři: TODO (login, jméno)
 *
 * Debug option:
 *   --tokens   print the token stream to stdout instead of compiling
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "src/error.h"
#include "src/lexer.h"

int main(int argc, char **argv) {
    const bool dump_tokens = argc > 1 && strcmp(argv[1], "--tokens") == 0;

    Lexer lexer;
    lexer_init(&lexer, stdin);

    // TODO (milestone 4+): replace with parse -> semantic analysis -> codegen.
    // Until then the whole input is only run through the lexer.
    for (;;) {
        Token t = lexer_next(&lexer);
        if (dump_tokens) token_print(stdout, &t);
        const bool done = t.kind == TK_EOF;
        token_free(&t);
        if (done) break;
    }

    lexer_free(&lexer);
    return ERR_OK;
}
