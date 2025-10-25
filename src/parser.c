#include "parser.h"
#include <stdlib.h>

#include "list.h"

ASTNode *ASTNode_ctor();


void ASTNode_dtor(ASTNode *node) {
    if (node == NULL) return;
    if (node->children) {
        List_dtor(node->children);
    }
    free(node);
}
