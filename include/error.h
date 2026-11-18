/*
 * Implementace překladače imperativního jazyka IFJ25
 * Error codes and fatal error reporting.
 * Autoři: TODO (login, jméno)
 */

#ifndef IFJ25_ERROR_H
#define IFJ25_ERROR_H

#include <stddef.h>

typedef enum {
    ERR_OK = 0,
    ERR_LEX = 1,            // wrong structure of a lexeme
    ERR_SYNTAX = 2,         // wrong syntax, missing prolog/skeleton
    ERR_SEM_UNDEFINED = 3,  // use of undefined function or variable
    ERR_SEM_REDEFINED = 4,  // redefinition of function or variable
    ERR_SEM_ARGS = 5,       // wrong argument count / builtin argument type
    ERR_SEM_TYPES = 6,      // type incompatibility in expression
    ERR_SEM_OTHER = 10,
    ERR_INTERNAL = 99       // e.g. allocation failure
} ErrorCode;

// Prints the message to stderr and terminates the compiler with `code`.
// The compiler stops at the first error, so no cleanup is needed on error paths.
// line <= 0 means "no source position".
_Noreturn void fatal(ErrorCode code, int line, int col, const char *fmt, ...);

// Allocation wrappers: exit with ERR_INTERNAL instead of returning NULL.
void *xmalloc(size_t size);
void *xrealloc(void *ptr, size_t size);

#endif
