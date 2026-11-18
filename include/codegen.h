/*
 * Implementace překladače imperativního jazyka IFJ25
 * Code generation: IFJcode25.
 * Autoři: TODO (login, jméno)
 */

#ifndef IFJ25_CODEGEN_H
#define IFJ25_CODEGEN_H

#include <stdio.h>

#include "ast.h"

// Writes the IFJcode25 program for an analysed AST to `out`.
void generate_program(const Program *program, FILE *out);

#endif
