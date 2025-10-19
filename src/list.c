#include "list.h"
#include "parser.h"

#include <stdlib.h>

List *List_ctor(const size_t capacity) {
    List *list = malloc(sizeof(List));
    if (list == NULL) {
        return NULL;
    }
    list->data = malloc(capacity * sizeof(ASTNode *));
    if (list->data == NULL) {
        free(list);
        return NULL;
    }
    list->capacity = capacity;
    list->count = 0;
    return list;
}

void List_Add(List *list, const ASTNode *data) {
    if (list->count == list->capacity) {
        const size_t newCapacity = list->capacity * 2;
        ASTNode **newData = realloc(list->data, newCapacity * sizeof(ASTNode *));
        if (newData != NULL) {
            list->data = newData;
            list->capacity = newCapacity;
        } else {
            return;
        }
    }
    list->data[list->count++] = (ASTNode *) data;
}

void List_Clear(List *list) {
    for (size_t i = 0; i < list->count; i++) {
        ASTNode_dtor(list->data[i]);
    }
    list->count = 0;
}

void List_dtor(List *list) {
    List_Clear(list);
    free(list->data);
    free(list);
}
