/*
 * Implementace překladače imperativního jazyka IFJ25
 * Growable array of pointers.
 * Autoři: TODO (login, jméno)
 */

#include "vec.h"

#include <stdlib.h>

#include "error.h"

void vec_push(Vec *v, void *item) {
    if (v->count == v->capacity) {
        v->capacity = v->capacity ? v->capacity * 2 : 8;
        v->items = xrealloc(v->items, v->capacity * sizeof(void *));
    }
    v->items[v->count++] = item;
}

void vec_free(Vec *v) {
    free(v->items);
    v->items = NULL;
    v->count = v->capacity = 0;
}
