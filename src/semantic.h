/*
 * Implementace překladače imperativního jazyka IFJ25
 * Semantic analysis.
 * Autoři: TODO (login, jméno)
 */

#ifndef IFJ25_SEMANTIC_H
#define IFJ25_SEMANTIC_H

#include "ast.h"

// Checks the program and annotates the AST for code generation:
// resolves names (local / getter / setter / global), assigns frame names to
// local variables, collects global variables.
// On a semantic error the compiler exits with code 3, 4, 5, 6 or 10.
void analyse_program(Program *program);

// Builtin function description, shared with the code generator.
typedef enum { ARG_ANY, ARG_NUM, ARG_STRING, ARG_INT } ArgType; // ARG_INT: Num with integer value

typedef struct {
    const char *name;
    int arity;
    ArgType args[3];
} Builtin;

// Returns NULL for an unknown builtin.
const Builtin *builtin_find(const char *name);

#endif
