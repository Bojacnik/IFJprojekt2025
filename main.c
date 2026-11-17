/*
 * Implementace překladače imperativního jazyka IFJ25
 * Entry point: reads IFJ25 source from stdin, writes IFJcode25 to stdout.
 * Autoři: TODO (login, jméno)
 *
 * Debug option:
 *   --tokens   only run the lexer and print the token stream to stdout
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "codegen.h"
#include "error.h"
#include "lexer.h"
#include "parser.h"
#include "semantic.h"

static int dump_tokens(Lexer *lexer) {
    for (;;) {
        Token t = lexer_next(lexer);
        token_print(stdout, &t);
        const bool done = t.kind == TK_EOF;
        token_free(&t);
        if (done) return ERR_OK;
    }
}

int main(int argc, char **argv) {
    Lexer lexer;
    lexer_init(&lexer, stdin);

    if (argc > 1 && strcmp(argv[1], "--tokens") == 0) {
        const int code = dump_tokens(&lexer);
        lexer_free(&lexer);
        return code;
    }

    Program *program = parse_program(&lexer);
    lexer_free(&lexer);
    analyse_program(program);
    generate_program(program, stdout);
    program_free(program);
    return ERR_OK;
}
