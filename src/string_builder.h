#ifndef IFJCODE25_STRING_BUILDER_H
#define IFJCODE25_STRING_BUILDER_H

#include <stddef.h>

typedef struct StringBuilder {
    char *buffer;
    size_t capacity;
    size_t count;
} StringBuilder;

StringBuilder* StringBuilder_ctor(size_t capacity);

void StringBuilder_Add(StringBuilder *sb, char c);

void StringBuilder_Clear(StringBuilder *sb);

void StringBuilder_dtor(StringBuilder *sb, bool freeData);

#endif