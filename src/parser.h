#ifndef IFJCODE25_PARSER_H
#define IFJCODE25_PARSER_H
#include "list.h"
#include "token.h"

typedef struct ASTNode {
    Token token;
    List *children;
} ASTNode;

ASTNode *ASTNode_ctor(const Token token);

ASTNode *ASTNode_addChild(ASTNode *parent, const ASTNode *child);

void ASTNode_dtor(ASTNode *node);

ASTNode *parse();

#endif
