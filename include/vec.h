/*
 * Implementace překladače imperativního jazyka IFJ25
 * Growable array of pointers.
 * Autoři: TODO (login, jméno)
 */

#ifndef IFJ25_VEC_H
#define IFJ25_VEC_H

#include <stddef.h>

typedef struct {
    void **items;
    size_t count;
    size_t capacity;
} Vec;

#define VEC_INIT {NULL, 0, 0}

void vec_push(Vec *v, void *item);

// Frees the array itself (not the items).
void vec_free(Vec *v);

#endif
