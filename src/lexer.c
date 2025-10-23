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
    LS_IDENTIFIERORKEYWORD,
    LS_INTORFLOAT,
    LS_STRING,
    LS_FLOAT,
    LS_CANBECOMMENTORDIVIDE,
    LS_COMMENT,
    LS_MULTILINE_COMMENT,
    LS_CANBEGREATERORGREATEROREQUAL,
    LS_CANBELESSERORLESSOREQUAL,
    LS_CANBEASSIGNOREQUALS,
    LS_INBUILTFUNCTION,
    LS_CANBESPECIALCHARACTERINSTRING,
    LS_CANBEMULTILITECOMMENTEND,
} LEXER_STATE;

bool isHexadecimal(const char c) { return isdigit(c) || (c <= 'F' && c >= 'A') || (c <= 'f' && c >= 'a'); }

// Trie Tree impl
KEYWORD_TYPE isKeyword(const char *s) {
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
                default:
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
                default:
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
        default: break;
    }

    return KWTYPE_NONE;
}

INBUILTFUNCTION_TYPE isInbuiltFunction(const char *s) {
    if (strcmp(s, "str") == 0) return INBUILT_STRING;
    if (strcmp(s, "write") == 0) return INBUILT_WRITE;
    if (strcmp(s, "read_num") == 0) return INBUILT_READNUM;
    if (strcmp(s, "floor") == 0) return INBUILT_FLOOR;
    return INBUILT_NONE;
}

inline bool isInbuiltFunctionPrefix(const char *s) {
    return strcmp(s, "Ifj") == 0;
}

inline int getMaxInbuiltFunctionLength() {
    return 8; // read_num
}

ErrorOrToken GetNextToken(FILE *source) {
    int c;
    LEXER_STATE state = LS_NONE;
    StringBuilder *sb = StringBuilder_ctor(2);

    if (sb == nullptr) {
        ErrorOrToken result;
        result.isError = true;
        result.errorType = ERROR_OTHER;
        return result;
    }

    while ((c = fgetc(source)) != EOF) {
        switch ((char) c) {
            case '\\':
                switch (state) {
                        // in string, escaped \
                        case LS_CANBESPECIALCHARACTERINSTRING:
                        StringBuilder_Add(sb, (char) c);
                        break;
                    case LS_STRING:
                        state = LS_CANBESPECIALCHARACTERINSTRING;
                        break;
                    case LS_CANBECOMMENTORDIVIDE:
                        ungetc(c, source);
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_OPERATOR, .operator_type = OPTYPE_DIVIDE}
                        };
                    case LS_COMMENT:
                    case LS_MULTILINE_COMMENT:
                        break;
                    default:
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){.isError = true, .errorType = ERROR_LEXICAL};
                }
                break;
            case 't': {
                switch (state) {
                    case LS_NONE:
                        state = LS_IDENTIFIERORKEYWORD;
                        StringBuilder_Add(sb, (char) c);
                        break;
                    case LS_CANBESPECIALCHARACTERINSTRING:
                        StringBuilder_Add(sb, '\t');
                        state = LS_STRING;
                        break;
                    case LS_STRING:
                        StringBuilder_Add(sb, c);
                        break;
                    case LS_CANBECOMMENTORDIVIDE:
                        ungetc(c, source);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_OPERATOR, .operator_type = OPTYPE_DIVIDE}
                        };
                    case LS_INBUILTFUNCTION:
                    case LS_IDENTIFIERORKEYWORD:
                        StringBuilder_Add(sb, (char) c);
                        break;
                    case LS_COMMENT:
                    case LS_MULTILINE_COMMENT:
                        break;
                    default:
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){.isError = true, .errorType = ERROR_LEXICAL};
                }
                break;
            }

            case '0': {
                switch (state) {
                    // Can't start number with 0 unless it's just 0
                    case LS_NONE:
                        state = LS_INTORFLOAT;
                        StringBuilder_Add(sb, (char) c);
                        break;
                    case LS_CANBESPECIALCHARACTERINSTRING:
                        StringBuilder_Add(sb, '\0');
                        state = LS_STRING;
                        break;
                    case LS_STRING:
                        StringBuilder_Add(sb, (char) c);
                        break;
                    case LS_CANBECOMMENTORDIVIDE:
                        ungetc(c, source);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_OPERATOR, .operator_type = OPTYPE_DIVIDE}
                        };
                    case LS_FLOAT:
                    case LS_INTORFLOAT:
                    case LS_IDENTIFIERORKEYWORD:
                        StringBuilder_Add(sb, (char) c);
                        break;
                    case LS_COMMENT:
                    case LS_MULTILINE_COMMENT:
                        break;
                    default:
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){.isError = true, .errorType = ERROR_LEXICAL};
                }
                break;
            }

            case 'n':
                switch (state) {
                    case LS_NONE:
                        state = LS_IDENTIFIERORKEYWORD;
                        StringBuilder_Add(sb, (char) c);
                        break;
                    case LS_CANBESPECIALCHARACTERINSTRING:
                        StringBuilder_Add(sb, '\n');
                        state = LS_STRING;
                        break;
                    case LS_STRING:
                        StringBuilder_Add(sb, (char) c);
                        break;
                    case LS_CANBECOMMENTORDIVIDE:
                        ungetc(c, source);
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_OPERATOR, .operator_type = OPTYPE_DIVIDE}
                        };
                    case LS_INBUILTFUNCTION:
                    case LS_IDENTIFIERORKEYWORD:
                        StringBuilder_Add(sb, (char) c);
                        break;
                    case LS_COMMENT:
                    case LS_MULTILINE_COMMENT:
                        break;
                    default:
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){.isError = true, .errorType = ERROR_LEXICAL};
                }
                break;

            case '/': {
                switch (state) {
                    case LS_NONE:
                        state = LS_CANBECOMMENTORDIVIDE;
                        break;

                    case LS_CANBECOMMENTORDIVIDE: {
                        state = LS_COMMENT;
                        break;
                    }
                    case LS_COMMENT: {
                        // stay in comment
                        break;
                    }
                    case LS_MULTILINE_COMMENT: {
                        // stay in multiline comment
                        break;
                    case LS_CANBEMULTILITECOMMENTEND:
                        state = LS_NONE;
                        break;
                    default:
                        StringBuilder_Add(sb, (char) c);
                        break;
                    }
                }
                break;
            }
            case '*': {
                switch (state) {
                    case LS_NONE: {
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_OPERATOR, .operator_type = OPTYPE_MULTIPLY}
                        };
                    }
                    case LS_CANBECOMMENTORDIVIDE:
                        state = LS_MULTILINE_COMMENT;
                        break;
                    case LS_COMMENT:
                        break;
                    case LS_MULTILINE_COMMENT:
                        state = LS_CANBEMULTILITECOMMENTEND;
                        break;
                    case LS_CANBEMULTILITECOMMENTEND:
                        state = LS_MULTILINE_COMMENT;
                    case LS_STRING:
                    case LS_IDENTIFIERORKEYWORD: {
                        StringBuilder_Add(sb, (char) c);
                        break;
                    }
                    default:
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){.isError = true, .errorType = ERROR_LEXICAL};
                }
                break;
            }
            case '.':
                switch (state) {
                    case LS_INTORFLOAT:
                        state = LS_FLOAT;
                        StringBuilder_Add(sb, (char) c);
                        break;
                    case LS_CANBECOMMENTORDIVIDE:
                        ungetc(c, source);
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_OPERATOR, .operator_type = OPTYPE_DIVIDE}
                        };
                    case LS_CANBEMULTILITECOMMENTEND:
                        state = LS_MULTILINE_COMMENT;
                        break;
                    case LS_IDENTIFIERORKEYWORD: {
                        char *strId = StringBuilder_ToString(sb);

                        if (strcmp(strId, "Ifj") == 0) {
                            StringBuilder_Clear(sb);
                            state = LS_INBUILTFUNCTION;
                            break;
                        }

                        StringBuilder_dtor(sb);
                        const KEYWORD_TYPE kw = isKeyword(strId);
                        if (kw != KWTYPE_NONE) {
                            return (ErrorOrToken){
                                .isError = false, .token = {.type = TKTYPE_KEYWORD, .keyword_type = kw}
                            };
                        }
                        return (ErrorOrToken){
                            .isError = false, .token = {.type = TKTYPE_IDENTIFIER, .identifier = strId}
                        };
                    }
                    case LS_COMMENT:
                    case LS_MULTILINE_COMMENT:
                        break;
                    default:
                        StringBuilder_Add(sb, (char) c);
                        break;
                }
                break;

            // if newline, space, tab or carriage return
            case '\n':
            case ' ':
            case '\t':
            case '\r':
                ErrorOrToken token;
                switch (state) {
                    case LS_NONE:
                        continue;
                    case LS_CANBEMULTILITECOMMENTEND:
                        state = LS_MULTILINE_COMMENT;
                        break;
                    case LS_IDENTIFIERORKEYWORD:
                        token.isError = false;
                        char *strId = StringBuilder_ToString(sb);
                        StringBuilder_dtor(sb);
                        const KEYWORD_TYPE kw = isKeyword(strId);
                        if (kw != KWTYPE_NONE) {
                            return (ErrorOrToken){
                                .isError = false, .token = {.type = TKTYPE_KEYWORD, .keyword_type = kw}
                            };
                        }
                        return (ErrorOrToken){
                            .isError = false, .token = {.type = TKTYPE_IDENTIFIER, .identifier = strId}
                        };
                    case LS_CANBEASSIGNOREQUALS:
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_OPERATOR, .operator_type = OPTYPE_ASSIGN}
                        };
                    case LS_CANBEGREATERORGREATEROREQUAL:
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_OPERATOR, .operator_type = OPTYPE_GREATER}
                        };
                    case LS_CANBELESSERORLESSOREQUAL:
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_OPERATOR, .operator_type = OPTYPE_LESS}
                        };
                    case LS_INTORFLOAT:
                        if (isHexadecimal((char) c)) {
                            state = LS_FLOAT;
                            StringBuilder_Add(sb, (char) c);
                            break;
                        }
                        token.isError = false;
                        const char *strInt = StringBuilder_ToString(sb);
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false, .token = {.type = TKTYPE_LITERAL_INT, .int_value = atoi(strInt)}
                        };
                        break;
                    case LS_FLOAT:
                        token.isError = false;
                        const char *strFloat = StringBuilder_ToString(sb);
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_LITERAL_FLOAT, .float_value = (float) atof(strFloat)}
                        };
                    case LS_CANBECOMMENTORDIVIDE:
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_OPERATOR, .operator_type = OPTYPE_DIVIDE}
                        };
                    case LS_COMMENT:
                        if (c == '\n') {
                            state = LS_NONE;
                        }
                        break;
                    case LS_MULTILINE_COMMENT:
                        if (c == '/') {
                            state = LS_NONE;
                        }
                        break;
                    case LS_STRING:
                        StringBuilder_Add(sb, (char) c);
                        break;
                    default:
                        return (ErrorOrToken){.isError = true, .errorType = ERROR_LEXICAL};;
                }
                break;

            // is string literal
            case '"':
                switch (state) {
                    case LS_NONE:
                        state = LS_STRING;
                        break;
                    case LS_CANBEMULTILITECOMMENTEND:
                        state = LS_MULTILINE_COMMENT;
                        break;
                    case LS_STRING:
                        char *strStr = StringBuilder_ToString(sb);
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false, .token = {.type = TKTYPE_LITERAL_STRING, .string_value = strStr}
                        };
                    case LS_CANBECOMMENTORDIVIDE:
                        ungetc(c, source);
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_OPERATOR, .operator_type = OPTYPE_DIVIDE}
                        };
                    case LS_COMMENT:
                    case LS_MULTILINE_COMMENT:
                        break;
                    case LS_IDENTIFIERORKEYWORD:
                    default:
                        return (ErrorOrToken){.isError = true, .errorType = ERROR_LEXICAL};
                }
                break;

            // is identifier or keyword
            case '_':
            case 'a' ... 'm':
            case 'o' ... 's':
            case 'u' ... 'z':
            case 'A' ... 'Z':
                switch (state) {
                    case LS_NONE:
                        state = LS_IDENTIFIERORKEYWORD;
                        StringBuilder_Add(sb, (char) c);
                        break;
                    case LS_CANBEMULTILITECOMMENTEND:
                        state = LS_MULTILINE_COMMENT;
                        break;
                    case LS_IDENTIFIERORKEYWORD:
                        StringBuilder_Add(sb, (char) c);
                        break;
                    case LS_INTORFLOAT:
                    case LS_FLOAT:
                        // ERROR
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){.isError = true, .errorType = ERROR_LEXICAL};
                    case LS_STRING:
                        StringBuilder_Add(sb, (char) c);
                        break;
                    case LS_CANBECOMMENTORDIVIDE:
                        ungetc(c, source);
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_OPERATOR, .operator_type = OPTYPE_DIVIDE}
                        };
                    case LS_INBUILTFUNCTION:
                        StringBuilder_Add(sb, (char) c);
                        break;
                    case LS_COMMENT:
                    case LS_MULTILINE_COMMENT:
                        break;
                    default:
                        return (ErrorOrToken){.isError = true, .errorType = ERROR_LEXICAL};
                }
                break;

            // is digit
            case '1' ... '9':
                switch (state) {
                    case LS_NONE:
                        state = LS_INTORFLOAT;
                        StringBuilder_Add(sb, (char) c);
                        break;
                    case LS_CANBEMULTILITECOMMENTEND:
                        state = LS_MULTILINE_COMMENT;
                        break;
                    case LS_IDENTIFIERORKEYWORD:
                    case LS_INTORFLOAT:
                    case LS_FLOAT:
                    case LS_STRING:
                        StringBuilder_Add(sb, (char) c);
                        break;
                    case LS_CANBECOMMENTORDIVIDE:
                        ungetc(c, source);
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_OPERATOR, .operator_type = OPTYPE_DIVIDE}
                        };
                    case LS_COMMENT:
                    case LS_MULTILINE_COMMENT:
                        break;
                    default:
                        return (ErrorOrToken){.isError = true, .errorType = ERROR_LEXICAL};
                }
                break;
            case '{':
                switch (state) {
                    case LS_NONE:
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_PUNCTUATION, .punctuation_type = PTTYPE_OPENBRACE}
                        };
                    case LS_CANBEMULTILITECOMMENTEND:
                        state = LS_MULTILINE_COMMENT;
                        break;
                    case LS_STRING:
                        StringBuilder_Add(sb, (char) c);
                        break;
                    case LS_CANBECOMMENTORDIVIDE:
                        ungetc(c, source);
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_OPERATOR, .operator_type = OPTYPE_DIVIDE}
                        };
                    case LS_COMMENT:
                    case LS_MULTILINE_COMMENT:
                        break;

                    default:
                        return (ErrorOrToken){.isError = true, .errorType = ERROR_LEXICAL};
                }
                break;
            case '}':
                switch (state) {
                    case LS_NONE:
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_PUNCTUATION, .punctuation_type = PTTYPE_CLOSEBRACE}
                        };
                    case LS_CANBEMULTILITECOMMENTEND:
                        state = LS_MULTILINE_COMMENT;
                        break;
                    case LS_STRING:
                        StringBuilder_Add(sb, (char) c);
                        break;
                    case LS_CANBECOMMENTORDIVIDE:
                        ungetc(c, source);
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_OPERATOR, .operator_type = OPTYPE_DIVIDE}
                        };
                    case LS_COMMENT:
                    case LS_MULTILINE_COMMENT:
                        break;
                    default:
                        return (ErrorOrToken){.isError = true, .errorType = ERROR_LEXICAL};
                }
                break;
            case '(':
                switch (state) {
                    case LS_NONE:
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_PUNCTUATION, .punctuation_type = PTTYPE_OPENPARENTHESIS}
                        };
                    case LS_CANBEMULTILITECOMMENTEND:
                        state = LS_MULTILINE_COMMENT;
                        break;
                    case LS_STRING:
                        StringBuilder_Add(sb, (char) c);
                        break;
                    case LS_IDENTIFIERORKEYWORD:
                        ungetc(c, source);
                        char *strId = StringBuilder_ToString(sb);
                        StringBuilder_dtor(sb);
                        const KEYWORD_TYPE kw = isKeyword(strId);
                        if (kw != KWTYPE_NONE) {
                            return (ErrorOrToken){
                                .isError = false, .token = {.type = TKTYPE_KEYWORD, .keyword_type = kw}
                            };
                        }
                        return (ErrorOrToken){
                            .isError = false, .token = {.type = TKTYPE_IDENTIFIER, .identifier = strId}
                        };
                    case LS_CANBECOMMENTORDIVIDE:
                        ungetc(c, source);
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_OPERATOR, .operator_type = OPTYPE_DIVIDE}
                        };
                    case LS_INBUILTFUNCTION:
                        ungetc(c, source);
                        char *strInbuilt = StringBuilder_ToString(sb);
                        StringBuilder_dtor(sb);
                        const INBUILTFUNCTION_TYPE ibf = isInbuiltFunction(strInbuilt);
                        free(strInbuilt);
                        if (ibf != INBUILT_NONE) {
                            return (ErrorOrToken){
                                .isError = false,
                                .token = {.type = TKTYPE_INBUILTFUNCTION, .inbuilt_function_type = ibf}
                            };
                        }
                        return (ErrorOrToken){
                            .isError = true,
                            .errorType = ERROR_LEXICAL
                        };
                    case LS_COMMENT:
                    case LS_MULTILINE_COMMENT:
                        break;
                    default:
                        return (ErrorOrToken){.isError = true, .errorType = ERROR_LEXICAL};
                }
                break;
            case ')':
                switch (state) {
                    case LS_NONE:
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_PUNCTUATION, .punctuation_type = PTTYPE_CLOSEPARENTHESIS}
                        };
                    case LS_CANBEMULTILITECOMMENTEND:
                        state = LS_MULTILINE_COMMENT;
                        break;
                    case LS_STRING:
                        StringBuilder_Add(sb, (char) c);
                        break;
                    case LS_IDENTIFIERORKEYWORD:
                        ungetc(c, source);
                        char *strId = StringBuilder_ToString(sb);
                        StringBuilder_dtor(sb);
                        const KEYWORD_TYPE kw = isKeyword(strId);
                        if (kw != KWTYPE_NONE) {
                            return (ErrorOrToken){
                                .isError = false, .token = {.type = TKTYPE_KEYWORD, .keyword_type = kw}
                            };
                        }
                        return (ErrorOrToken){
                            .isError = false, .token = {.type = TKTYPE_IDENTIFIER, .identifier = strId}
                        };
                    case LS_CANBECOMMENTORDIVIDE:
                        ungetc(c, source);
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_OPERATOR, .operator_type = OPTYPE_DIVIDE}
                        };
                    case LS_COMMENT:
                    case LS_MULTILINE_COMMENT:
                        break;
                    case LS_FLOAT:
                        ungetc(c, source);
                        char *strFloat = StringBuilder_ToString(sb);
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_LITERAL_FLOAT, .float_value = (float) atof(strFloat)}
                        };
                    case LS_INTORFLOAT:
                        ungetc(c, source);
                        char *strInt = StringBuilder_ToString(sb);
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false, .token = {.type = TKTYPE_LITERAL_INT, .int_value = atoi(strInt)}
                        };
                    default:
                        return (ErrorOrToken){.isError = true, .errorType = ERROR_LEXICAL};
                }
                break;
            case ',':
                switch (state) {
                    case LS_NONE:
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_PUNCTUATION, .punctuation_type = PTTYPE_COMMA}
                        };
                    case LS_CANBEMULTILITECOMMENTEND:
                        state = LS_MULTILINE_COMMENT;
                        break;
                    case LS_STRING:
                        StringBuilder_Add(sb, (char) c);
                        break;
                    case LS_IDENTIFIERORKEYWORD:
                        ungetc(c, source);
                        char *strId = StringBuilder_ToString(sb);
                        StringBuilder_dtor(sb);
                        const KEYWORD_TYPE kw = isKeyword(strId);
                        if (kw != KWTYPE_NONE) {
                            return (ErrorOrToken){
                                .isError = false, .token = {.type = TKTYPE_KEYWORD, .keyword_type = kw}
                            };
                        }
                        return (ErrorOrToken){
                            .isError = false, .token = {.type = TKTYPE_IDENTIFIER, .identifier = strId}
                        };
                    case LS_CANBECOMMENTORDIVIDE:
                        ungetc(c, source);
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_OPERATOR, .operator_type = OPTYPE_DIVIDE}
                        };
                    case LS_COMMENT:
                    case LS_MULTILINE_COMMENT:
                        break;
                    default:
                        return (ErrorOrToken){.isError = true, .errorType = ERROR_LEXICAL};
                }
                break;
            case ';':
                switch (state) {
                    case LS_NONE:
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_PUNCTUATION, .punctuation_type = PTTYPE_SEMICOLON}
                        };
                    case LS_CANBEMULTILITECOMMENTEND:
                        state = LS_MULTILINE_COMMENT;
                        break;
                    case LS_CANBECOMMENTORDIVIDE:
                        ungetc(c, source);
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_OPERATOR, .operator_type = OPTYPE_DIVIDE}
                        };
                    case LS_STRING:
                        StringBuilder_Add(sb, (char) c);
                        break;
                    case LS_COMMENT:
                    case LS_MULTILINE_COMMENT:
                        break;
                    default:
                        return (ErrorOrToken){.isError = true, .errorType = ERROR_LEXICAL};
                }
                break;
            case '+':
                switch (state) {
                    case LS_NONE:
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_OPERATOR, .operator_type = OPTYPE_PLUS}
                        };
                    case LS_CANBEMULTILITECOMMENTEND:
                        state = LS_MULTILINE_COMMENT;
                        break;
                    case LS_STRING:
                    case LS_IDENTIFIERORKEYWORD:
                        StringBuilder_Add(sb, (char) c);
                        break;
                    case LS_CANBECOMMENTORDIVIDE:
                        ungetc(c, source);
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_OPERATOR, .operator_type = OPTYPE_DIVIDE}
                        };
                    case LS_COMMENT:
                    case LS_MULTILINE_COMMENT:
                        break;
                    default:
                        return (ErrorOrToken){.isError = true, .errorType = ERROR_LEXICAL};
                }
                break;
            case '-':
                switch (state) {
                    case LS_NONE:
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_OPERATOR, .operator_type = OPTYPE_MINUS}
                        };
                    case LS_CANBEMULTILITECOMMENTEND:
                        state = LS_MULTILINE_COMMENT;
                        break;
                    case LS_STRING:
                    case LS_IDENTIFIERORKEYWORD:
                        StringBuilder_Add(sb, (char) c);
                        break;
                    case LS_CANBECOMMENTORDIVIDE:
                        ungetc(c, source);
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_OPERATOR, .operator_type = OPTYPE_DIVIDE}
                        };
                    case LS_COMMENT:
                    case LS_MULTILINE_COMMENT:
                        break;
                    default:
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){.isError = true, .errorType = ERROR_LEXICAL};
                }
                break;
            case '>':
                switch (state) {
                    case LS_NONE:
                        state = LS_CANBEGREATERORGREATEROREQUAL;
                        break;

                    case LS_CANBEMULTILITECOMMENTEND:
                        state = LS_MULTILINE_COMMENT;
                        break;

                    case LS_CANBEGREATERORGREATEROREQUAL:
                        ungetc(c, source);
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_OPERATOR, .operator_type = OPTYPE_GREATER}
                        };
                    case LS_CANBELESSERORLESSOREQUAL:
                        ungetc(c, source);
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_OPERATOR, .operator_type = OPTYPE_LESS}
                        };

                    case LS_STRING:
                    case LS_IDENTIFIERORKEYWORD:
                        StringBuilder_Add(sb, (char) c);
                        break;
                    case LS_CANBECOMMENTORDIVIDE:
                        ungetc(c, source);
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_OPERATOR, .operator_type = OPTYPE_DIVIDE}
                        };
                    case LS_COMMENT:
                    case LS_MULTILINE_COMMENT:
                        break;
                    default:
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){.isError = true, .errorType = ERROR_LEXICAL};
                }
                break;

            case '<':
                switch (state) {
                    case LS_NONE:
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_OPERATOR, .operator_type = OPTYPE_LESS}
                        };
                    case LS_CANBEMULTILITECOMMENTEND:
                        state = LS_MULTILINE_COMMENT;
                        break;
                    case LS_CANBEGREATERORGREATEROREQUAL:
                        ungetc(c, source);
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_OPERATOR, .operator_type = OPTYPE_GREATER}
                        };
                    case LS_CANBELESSERORLESSOREQUAL:
                        ungetc(c, source);
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_OPERATOR, .operator_type = OPTYPE_LESS}
                        };
                    case LS_STRING:
                    case LS_IDENTIFIERORKEYWORD:
                        StringBuilder_Add(sb, (char) c);
                        break;
                    case LS_CANBECOMMENTORDIVIDE:
                        ungetc(c, source);
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_OPERATOR, .operator_type = OPTYPE_DIVIDE}
                        };
                    case LS_COMMENT:
                    case LS_MULTILINE_COMMENT:
                        break;
                    default:
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){.isError = true, .errorType = ERROR_LEXICAL};
                }
                break;

            case '=':
                switch (state) {
                    case LS_NONE:
                        state = LS_CANBEASSIGNOREQUALS;
                        break;
                    case LS_CANBEMULTILITECOMMENTEND:
                        state = LS_MULTILINE_COMMENT;
                        break;
                    case LS_CANBEASSIGNOREQUALS:
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_OPERATOR, .operator_type = OPTYPE_ASSIGN}
                        };
                    case LS_CANBELESSERORLESSOREQUAL:
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_OPERATOR, .operator_type = OPTYPE_LESSEQUAL}
                        };
                    case LS_CANBEGREATERORGREATEROREQUAL:
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_OPERATOR, .operator_type = OPTYPE_GREATEREQUAL}
                        };

                    case LS_STRING:
                    case LS_IDENTIFIERORKEYWORD:
                        StringBuilder_Add(sb, (char) c);
                        break;
                    case LS_CANBECOMMENTORDIVIDE:
                        ungetc(c, source);
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_OPERATOR, .operator_type = OPTYPE_DIVIDE}
                        };
                    case LS_COMMENT:
                    case LS_MULTILINE_COMMENT:
                        break;
                    default:
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){.isError = true, .errorType = ERROR_LEXICAL};
                }
                break;

            default:
                switch (state) {
                    case LS_NONE:
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_OPERATOR, .operator_type = OPTYPE_NOT}
                        };
                    case LS_CANBEASSIGNOREQUALS:
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_OPERATOR, .operator_type = OPTYPE_ASSIGN}
                        };
                    case LS_CANBELESSERORLESSOREQUAL:
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_OPERATOR, .operator_type = OPTYPE_LESS}
                        };
                    case LS_CANBEGREATERORGREATEROREQUAL:
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_OPERATOR, .operator_type = OPTYPE_GREATER}
                        };
                    case LS_STRING:
                    case LS_IDENTIFIERORKEYWORD:
                        StringBuilder_Add(sb, (char) c);
                        break;
                    case LS_CANBECOMMENTORDIVIDE:
                        ungetc(c, source);
                        StringBuilder_dtor(sb);
                        return (ErrorOrToken){
                            .isError = false,
                            .token = {.type = TKTYPE_OPERATOR, .operator_type = OPTYPE_DIVIDE}
                        };
                    case LS_COMMENT:
                    case LS_MULTILINE_COMMENT:
                        break;

                    default:
                        switch (state) {
                            default: StringBuilder_dtor(sb);
                                return (ErrorOrToken){.isError = true, .errorType = ERROR_LEXICAL};
                        }
                }
        }
    }
    // EOF reached
    StringBuilder_dtor(sb);
    return (ErrorOrToken){.isError = false, .token = {.type = TKTYPE_EOF}};
}
