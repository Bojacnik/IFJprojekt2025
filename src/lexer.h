#ifndef IFJCODE25_LEXER_H
#define IFJCODE25_LEXER_H

#include <stdio.h>

struct ErrorOrToken;

struct ErrorOrToken GetNextToken(FILE *source);

#endif
