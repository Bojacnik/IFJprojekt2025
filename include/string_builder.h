/*
 * Implementace překladače imperativního jazyka IFJ25
 * Growable byte buffer, always NUL-terminated.
 * Autoři: TODO (login, jméno)
 */

#ifndef IFJ25_STRING_BUILDER_H
#define IFJ25_STRING_BUILDER_H

#include <stddef.h>

typedef struct StringBuilder {
    char *buffer;    // always NUL-terminated
    size_t capacity; // bytes available for content (excluding the terminator)
    size_t count;    // bytes of content (may contain '\0' from "\x00")
} StringBuilder;

StringBuilder *StringBuilder_ctor(size_t capacity);

void StringBuilder_Add(StringBuilder *sb, char c);

void StringBuilder_AddStr(StringBuilder *sb, const char *s);

void StringBuilder_Clear(StringBuilder *sb);

// Borrowed view of the content, valid until the next modification.
const char *StringBuilder_CStr(const StringBuilder *sb);

// Heap copy of the content; the caller owns it.
char *StringBuilder_ToString(const StringBuilder *sb);

void StringBuilder_dtor(StringBuilder *sb);

#endif
