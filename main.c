#include <stdio.h>

#include "src/error.h"
#include "src/lexer.h"

void PrintToken(const TokenType tokenType, const Token *token) {
    switch (tokenType) {
        case TKTYPE_EOF:
            printf("Token: EOF\n");
            break;
        case TKTYPE_INBUILTFUNCTION:
            printf("Token: INBUILTFUNCTION");
            switch (token->inbuilt_function_type) {
                case INBUILT_STRING:
                    printf(" STRING\n");
                    break;
                case INBUILT_WRITE:
                    printf(" WRITE\n");
                    break;
                case INBUILT_READNUM:
                    printf(" READ_NUM\n");
                    break;
                default:
                    break;
            }
            break;
        case TKTYPE_KEYWORD:
            printf("Token: KEYWORD");
            switch (token->keyword_type) {
                case KWTYPE_IMPORT:
                    printf(" IMPORT\n");
                    break;
                case KWTYPE_FOR:
                    printf(" FOR\n");
                    break;
                case KWTYPE_CLASS:
                    printf(" CLASS\n");
                    break;
                case KWTYPE_STATIC:
                    printf(" STATIC\n");
                    break;
                case KWTYPE_IF:
                    printf(" IF\n");
                    break;
                case KWTYPE_ELSE:
                    printf(" ELSE\n");
                    break;
                case KWTYPE_WHILE:
                    printf(" WHILE\n");
                    break;
                case KWTYPE_IFJ:
                    printf(" IFJ\n");
                    break;
                case KWTYPE_IS:
                    printf(" IS\n");
                    break;
                case KWTYPE_NULL:
                    printf(" NULL\n");
                    break;
                case KWTYPE_NUM:
                    printf(" NUM\n");
                    break;
                case KWTYPE_RETURN:
                    printf(" RETURN\n");
                    break;
                case KWTYPE_STRING:
                    printf(" STRING\n");
                    break;
                case KWTYPE_VAR:
                    printf(" VAR\n");
                    break;
                default:
                    break;
            }
            break;
        case TKTYPE_IDENTIFIER:
            printf("Token: IDENTIFIER");
            printf(" %s\n", token->identifier);
            break;
        case TKTYPE_PUNCTUATION:
            printf("Token: PUNCTUATION");
            switch (token->punctuation_type) {
                case PTTYPE_OPENPARENTHESIS:
                    printf(" Type: OPENPARENTHESIS\n");
                    break;
                case PTTYPE_CLOSEPARENTHESIS:
                    printf(" Type: CLOSEPARENTHESIS\n");
                    break;
                case PTTYPE_OPENBRACKET:
                    printf(" Type: OPENBRACKET\n");
                    break;
                case PTTYPE_CLOSEBRACKET:
                    printf(" Type: CLOSEBRACKET\n");
                    break;
                case PTTYPE_OPENBRACE:
                    printf(" Type: OPENBRACE\n");
                    break;
                case PTTYPE_CLOSEBRACE:
                    printf(" Type: CLOSEBRACE\n");
                    break;
                case PTTYPE_UNDERSCORE:
                    printf(" Type: UNDERSCORE\n");
                    break;
                case PTTYPE_COMMA:
                    printf(" Type: COMMA\n");
                    break;
                case PTTYPE_SEMICOLON:
                    printf(" Type: SEMICOLON\n");
                    break;
                default:
                    break;
            }
            break;
        case TKTYPE_OPERATOR:
            printf("Token: OPERATOR\n");
            break;
        case TKTYPE_VARIABLE:
            printf("Token: VARIABLE\n");
            break;
        case TKTYPE_LITERAL_INT:
            printf("Token: LITERAL_INT\n");
            break;
        case TKTYPE_LITERAL_FLOAT:
            printf("Token: LITERAL_FLOAT\n");
            break;
        case TKTYPE_LITERAL_STRING:
            printf("Token: LITERAL_STRING");
            printf(" %s\n", token->string_value);
            break;
        case TKTYPE_LITERAL_NIL:
            printf("Token: LITERAL_NIL\n");
            break;
        case TKTYPE_LITERAL_BOOL:
            printf("Token: LITERAL_BOOL\n");
            break;
        default:
            printf("Token: UNKNOWN\n");
            break;
    }
}

int main(void) {
    printf("Starting lexical analysis!\n");
    FILE *testfile = fopen("../examples/ex1-faktorial-iterativne.wren", "r");
    for (;;) {
        const ErrorOrToken errorOrToken = GetNextToken(testfile);
        if (errorOrToken.isError == false && errorOrToken.token.type == TKTYPE_EOF) {
            puts("Reached EOF");
            break;
        }
        if (errorOrToken.isError) {
            perror("Lexer Error");
            break;
        }
        PrintToken(errorOrToken.token.type, &errorOrToken.token);
    }
    return 0;
}
