#include <stdio.h>

#ifndef IFJCODE25_LEXER_H
#define IFJCODE25_LEXER_H

struct ErrorOrToken;

struct ErrorOrToken GetNextToken(FILE* source);

#endif