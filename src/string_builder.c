#include "string_builder.h"

#include <stdlib.h>
#include <string.h>

#define SB_MIN_CAPACITY 16

StringBuilder *StringBuilder_ctor(const size_t capacity) {
    const size_t cap = capacity ? capacity : SB_MIN_CAPACITY;
    StringBuilder *sb = malloc(sizeof(StringBuilder));
    if (!sb) return nullptr;
    sb->buffer = malloc(cap * sizeof(char));
    if (!sb->buffer) {
        free(sb);
        return nullptr;
    }
    sb->capacity = cap;
    sb->count = 0;
    return sb;
}

void StringBuilder_Add(StringBuilder *sb, const char c) {
    if (!sb) return;
    if (sb->count >= sb->capacity) {
        const size_t newCapacity = sb->capacity ? sb->capacity * 2 : SB_MIN_CAPACITY;
        char *tmp = realloc(sb->buffer, newCapacity * sizeof(char));
        if (!tmp) {
            /* realloc failed: leave buffer as-is and don't append */
            return;
        }
        sb->buffer = tmp;
        sb->capacity = newCapacity;
    }
    sb->buffer[sb->count++] = c;
}

char *StringBuilder_ToString(const StringBuilder *sb) {
    if (!sb) return nullptr;
    char *str = malloc((sb->count + 1) * sizeof(char));
    if (!str) return nullptr;
    memcpy(str, sb->buffer, sb->count * sizeof(char));
    str[sb->count] = '\0';
    return str;
}

void StringBuilder_Clear(StringBuilder *sb) {
    if (!sb) return;
    sb->count = 0;
}

void StringBuilder_dtor(StringBuilder *sb) {
    if (!sb) return;
    free(sb->buffer);
    free(sb);
}
