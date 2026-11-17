/*
 * Implementace překladače imperativního jazyka IFJ25
 * Error codes and fatal error reporting.
 * Autoři: TODO (login, jméno)
 */

#include "error.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void fatal(ErrorCode code, int line, int col, const char *fmt, ...) {
    fprintf(stderr, "error %d", code);
    if (line > 0) fprintf(stderr, " at %d:%d", line, col);
    fputs(": ", stderr);

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fputc('\n', stderr);
    exit(code);
}

void *xmalloc(size_t size) {
    void *p = malloc(size ? size : 1);
    if (p == NULL) fatal(ERR_INTERNAL, 0, 0, "out of memory");
    return p;
}

void *xrealloc(void *ptr, size_t size) {
    void *p = realloc(ptr, size ? size : 1);
    if (p == NULL) fatal(ERR_INTERNAL, 0, 0, "out of memory");
    return p;
}
