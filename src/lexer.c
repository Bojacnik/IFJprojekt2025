#include "lexer.h"

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
inline bool isdigit(const char c) { return c <= '9' && c >= '0'; }

inline KeywordType iskeyword(const char *c, size_t len) {
    if (strcmp(c, "import") == 0)
        return KWTYPE_IMPORT;
    if (strcmp(c, "for"))
        return KWTYPE_FOR;
    if (strcmp(c, "class"))
        return KWTYPE_CLASS;
    if (strcmp(c, "static") == 0)
        return KWTYPE_STATIC;
    if (strcmp(c, "if") == 0)
        return KWTYPE_IF;
    if (strcmp(c, "else") == 0)
        return KWTYPE_ELSE;
    if (strcmp(c, "while") == 0)
        return KWTYPE_WHILE;
    return KWTYPE_NONE;
}

ErrorOrToken GetNextToken(FILE *source) {
    int c;
    ErrorOrToken token;
    LEXER_STATE state = LS_NONE;
    StringBuilder *sb = nullptr;
    sb = StringBuilder_ctor(512);

    if (sb == nullptr) {
        ErrorOrToken result;
        result.isError = true;
        result.errorType = ERROR_OTHER;
        return result;
    }

    while ((c = fgetc(source) != EOF)) {
        // if newline skip
        if (c == '\n' || c == '\r' || c == ' ' || c == '\t') {
            switch (state) {
                case LS_NONE:
                    continue;
                case LS_IDENTIFIERKEYWORD:
                    token.isError = false;
                    char *strId = sb->buffer;
                    StringBuilder_dtor(sb, false);
                    const KeywordType kw = iskeyword(strId, sb->count);
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
