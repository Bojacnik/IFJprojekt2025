#ifndef IFJCODE25_ERROR_H
#define IFJCODE25_ERROR_H

#include "token.h"

typedef enum ErrorType {
    ERROR_LEXICAL = 1,
    ERROR_SYNTAX = 2,
    ERROR_SEMANTIC_USEOFUNDEFINED = 3,
    ERROR_SEMANTIC_REDEFINITION = 4,
    ERROR_SEMANTIC_STATIC_UNEXPECTEDPARAMETER = 5,
    ERROR_SEMANTIC_STATIC_UNEXPECTEDTYPE = 6,
    ERROR_SEMANTIC_OTHER = 10,
    ERROR_OTHER = 99
} ErrorType;

typedef struct ErrorOrToken {
    bool isError;
    union {
        ErrorType errorType;
        Token token;
    };
} ErrorOrToken;

#endif