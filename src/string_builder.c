#include "string_builder.h"

#include <stdlib.h>

StringBuilder* StringBuilder_ctor(const size_t capacity) {
    StringBuilder* sb = malloc(sizeof(StringBuilder));
    sb->buffer = malloc(capacity*sizeof(char));
    sb->capacity = capacity;
    return sb;
}

void StringBuilder_Add(StringBuilder *sb, char c) {
    if (sb->capacity == sb->count) {
        const size_t newCapacity = sb->capacity * 2;
        char* buffer = realloc(sb->buffer, newCapacity*sizeof(char));
        if (buffer != sb->buffer)
            free(sb->buffer);
        sb->buffer = buffer;
    }
}

void StringBuilder_Clear(StringBuilder *sb) {
    sb->count = 0;
}

void StringBuilder_dtor(StringBuilder *sb, bool freeData) {
    if (freeData) {
        free(sb->buffer);
    }
    free(sb);
}


