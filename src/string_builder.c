/*
 * Implementace překladače imperativního jazyka IFJ25
 * Growable byte buffer, always NUL-terminated.
 * Autoři: TODO (login, jméno)
 */

#include "string_builder.h"

#include <stdlib.h>
#include <string.h>

#include "error.h"

#define SB_MIN_CAPACITY 16

StringBuilder *StringBuilder_ctor(size_t capacity) {
    if (capacity < SB_MIN_CAPACITY) capacity = SB_MIN_CAPACITY;
    StringBuilder *sb = xmalloc(sizeof(StringBuilder));
    sb->buffer = xmalloc(capacity + 1);
    sb->buffer[0] = '\0';
    sb->capacity = capacity;
    sb->count = 0;
    return sb;
}

void StringBuilder_Add(StringBuilder *sb, char c) {
    if (sb->count == sb->capacity) {
        sb->capacity *= 2;
        sb->buffer = xrealloc(sb->buffer, sb->capacity + 1);
    }
    sb->buffer[sb->count++] = c;
    sb->buffer[sb->count] = '\0';
}

void StringBuilder_AddStr(StringBuilder *sb, const char *s) {
    while (*s) StringBuilder_Add(sb, *s++);
}

void StringBuilder_Clear(StringBuilder *sb) {
    sb->count = 0;
    sb->buffer[0] = '\0';
}

const char *StringBuilder_CStr(const StringBuilder *sb) {
    return sb->buffer;
}

char *StringBuilder_ToString(const StringBuilder *sb) {
    char *str = xmalloc(sb->count + 1);
    memcpy(str, sb->buffer, sb->count + 1);
    return str;
}

void StringBuilder_dtor(StringBuilder *sb) {
    if (sb == NULL) return;
    free(sb->buffer);
    free(sb);
}
