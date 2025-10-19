#include <stdio.h>

#include "src/error.h"
#include "src/lexer.h"

void PrintToken(TokenType tokenType)
{
    switch (tokenType)
    {
    case TKTYPE_EOF:
        printf("Token: EOF\n");
        break;
    case TKTYPE_KEYWORD:
        printf("Token: KEYWORD\n");
        break;
    case TKTYPE_IDENTIFIER:
        printf("Token: IDENTIFIER\n");
        break;
    case TKTYPE_PUNCTUATION:
        printf("Token: PUNCTUATION\n");
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
        printf("Token: LITERAL_STRING\n");
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

int main(void)
{
    printf("Starting lexical analysis!\n");
    for (;;)
    {
        FILE* testfile = fopen("../examples/ex0-vsechny-konstrukce.wren", "r");
        const ErrorOrToken errorOrToken = GetNextToken(testfile);
        if (errorOrToken.isError == false && errorOrToken.token.type == TKTYPE_EOF)
        {
            puts("Reached EOF");
            break;
        }
        if (errorOrToken.isError)
        {
            perror("Lexer Error");
            break;
        }
        PrintToken(errorOrToken.token.type);
    }
    return 0;
}
