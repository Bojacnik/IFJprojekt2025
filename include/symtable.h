/*
 * Implementace překladače imperativního jazyka IFJ25
 * Symbol table: height-balanced (AVL) binary search tree, variant vv-BVS.
 * Autoři: TODO (login, jméno)
 */

#ifndef IFJ25_SYMTABLE_H
#define IFJ25_SYMTABLE_H

#include <stdbool.h>
#include <stddef.h>

typedef struct SymNode {
    char *key;              // owned copy of the key
    void *value;            // not owned (see symtable_free)
    int height;             // height of the subtree, leaf = 1
    struct SymNode *left;
    struct SymNode *right;
} SymNode;

typedef struct {
    SymNode *root;
    size_t count;
} SymTable;

void symtable_init(SymTable *t);

// Inserts key -> value. Returns false (and changes nothing) if the key exists.
bool symtable_insert(SymTable *t, const char *key, void *value);

// Returns true if the key exists; stores its value to *value when value != NULL.
bool symtable_lookup(const SymTable *t, const char *key, void **value);

// Calls fn for every entry in ascending key order.
void symtable_foreach(const SymTable *t, void (*fn)(const char *key, void *value, void *ctx), void *ctx);

// Frees all nodes; free_value (may be NULL) is called for every value.
void symtable_free(SymTable *t, void (*free_value)(void *value));

#endif
