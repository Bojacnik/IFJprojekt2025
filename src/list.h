#ifndef IFJCODE25_LIST_H
#define IFJCODE25_LIST_H

#include <stddef.h>

#include "parser.h"

typedef struct List {
    ASTNode *data;
    size_t capacity;
    size_t count;
} List;

List *List_ctor(size_t capacity);

void List_Add(List *list, const ASTNode *data);

void List_Clear(List *list, bool freeData);

void List_dtor(List *list, bool freeData);

#endif
