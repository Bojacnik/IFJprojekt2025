#include "lexer.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "string_builder.h"
#include "token.h"
#include "error.h"

typedef enum LEXER_STATE {
    LS_NONE,
    LS_IDENTIFIERKEYWORD,
    LS_INTORFLOAT,
    LS_FLOAT,
} LEXER_STATE;

bool isletter(const char c) { return c <= 'Z' && c >= 'A' || c <= 'z' && c >= 'a'; }

// Trie Tree impl
KeywordType isKeyword(const char *s) {
    switch (s[0]) {
        case 'c':
            if (strcmp(s, "class") == 0) return KWTYPE_CLASS;
            break;

        case 'e':
            if (strcmp(s, "else") == 0) return KWTYPE_ELSE;
            break;

        case 'f':
            switch (s[1]) {
                case 'o':
                    if (strcmp(s, "for") == 0) return KWTYPE_FOR;
                    break;
            }
            break;

        case 'i':
            switch (s[1]) {
                case 'f': {
                    if (strcmp(s, "if") == 0) return KWTYPE_IF;
                    if (strcmp(s, "Ifj") == 0) return KWTYPE_IFJ;
                }
                break;
                case 'm':
                    if (strcmp(s, "import") == 0) return KWTYPE_IMPORT;
                    break;
                case 's':
                    if (strcmp(s, "is") == 0) return KWTYPE_IS;
                    break;
            }
            break;

        case 'n':
            if (strcmp(s, "null") == 0) return KWTYPE_NULL;
            break;

        case 'N': {
            if (strcmp(s, "Num") == 0) return KWTYPE_NUM;
            if (strcmp(s, "Null") == 0) return KWTYPE_NULL;
        }
        break;

        case 'r':
            if (strcmp(s, "return") == 0) return KWTYPE_RETURN;
            break;

        case 's':
            if (strcmp(s, "static") == 0) return KWTYPE_STATIC;
            break;

        case 'S':
            if (strcmp(s, "String") == 0) return KWTYPE_STRING;
            break;

        case 'v':
            if (strcmp(s, "var") == 0) return KWTYPE_VAR;
            break;

        case 'w':
            if (strcmp(s, "while") == 0) return KWTYPE_WHILE;
            break;
    }

    return KWTYPE_NONE;
}


ErrorOrToken GetNextToken(FILE *source) {
    int c;
    ErrorOrToken token;
    LEXER_STATE state = LS_NONE;
    StringBuilder *sb = StringBuilder_ctor(512);

    if (sb == nullptr) {
        ErrorOrToken result;
        result.isError = true;
        result.errorType = ERROR_OTHER;
        return result;
    }

    while ((c = fgetc(source) != EOF)) {
        // if newline, space, tab or carriage return
        if (c == '\n' || c == '\r' || c == ' ' || c == '\t') {
            switch (state) {
                case LS_NONE:
                    continue;
                case LS_IDENTIFIERKEYWORD:
                    token.isError = false;
                    char *strId = sb->buffer;
                    StringBuilder_dtor(sb, false);
                    const KeywordType kw = isKeyword(strId);
                    if (kw != KWTYPE_NONE) {
                        return (ErrorOrToken){.isError = false, .token = {.type = TKTYPE_KEYWORD, .keyword_type = kw}};
                    }
                    return (ErrorOrToken){.isError = false, .token = {.type = TKTYPE_IDENTIFIER, .identifier = strId}};
                case LS_INTORFLOAT:
                    token.isError = false;
                    char *strInt = sb->buffer;
                    StringBuilder_dtor(sb, false);
                    return (ErrorOrToken){
                        .isError = false, .token = {.type = TKTYPE_LITERAL_INT, .int_value = atoi(strInt)}
                    };
                case LS_FLOAT:
                    token.isError = false;
                    char *strFloat = sb->buffer;
                    StringBuilder_dtor(sb, false);
                    return (ErrorOrToken){
                        .isError = false, .token = {.type = TKTYPE_LITERAL_FLOAT, .float_value = (float) atof(strFloat)}
                    };
            }
            break;
        }
    }

    // if starts with letter, it's identifier or can be keyword
    if (isletter((char) c)) {
        switch (state) {
            case LS_NONE:
                state = LS_IDENTIFIERKEYWORD;
                StringBuilder_Add(sb, (char) c);
                break;
            case LS_IDENTIFIERKEYWORD:
                StringBuilder_Add(sb, (char) c);
                break;
            case LS_INTORFLOAT:
                // ERROR
                ErrorOrToken result;
                result.isError = true;
                result.errorType = ERROR_LEXICAL;
                StringBuilder_dtor(sb, true);
                return result;
            case LS_FLOAT:
                // ERROR
                ErrorOrToken result2;
                result2.isError = true;
                result2.errorType = ERROR_LEXICAL;
                StringBuilder_dtor(sb, true);
                return result2;
        }
    }

    if (isdigit((char) c)) {
        switch (state) {
            case LS_NONE:
                state = LS_INTORFLOAT;
                StringBuilder_Add(sb, (char) c);
                break;
            case LS_IDENTIFIERKEYWORD:
                StringBuilder_Add(sb, (char) c);
                break;
            case LS_INTORFLOAT:
                StringBuilder_Add(sb, (char) c);
                break;
            case LS_FLOAT:
                StringBuilder_Add(sb, (char) c);
                break;
        }
    }
}
