/*
 * Implementace překladače imperativního jazyka IFJ25
 * Symbol table: height-balanced (AVL) binary search tree, variant vv-BVS.
 * Autoři: TODO (login, jméno)
 *
 * After every insertion the heights of the two subtrees of each node on the
 * insertion path differ by at most one; a violation is repaired by a single
 * or double rotation, so lookup and insertion are O(log n).
 */

#include "symtable.h"

#include <stdlib.h>
#include <string.h>

#include "error.h"

static int height(const SymNode *n) { return n ? n->height : 0; }

static void update_height(SymNode *n) {
    const int hl = height(n->left), hr = height(n->right);
    n->height = 1 + (hl > hr ? hl : hr);
}

static int balance_factor(const SymNode *n) { return height(n->left) - height(n->right); }

/*      n              l
 *     / \            / \
 *    l   c   ==>    a   n
 *   / \                / \
 *  a   b              b   c
 */
static SymNode *rotate_right(SymNode *n) {
    SymNode *l = n->left;
    n->left = l->right;
    l->right = n;
    update_height(n);
    update_height(l);
    return l;
}

static SymNode *rotate_left(SymNode *n) {
    SymNode *r = n->right;
    n->right = r->left;
    r->left = n;
    update_height(n);
    update_height(r);
    return r;
}

static SymNode *rebalance(SymNode *n) {
    update_height(n);
    const int bf = balance_factor(n);
    if (bf > 1) {                       // left subtree too high
        if (balance_factor(n->left) < 0) n->left = rotate_left(n->left);   // LR case
        return rotate_right(n);                                            // LL case
    }
    if (bf < -1) {                      // right subtree too high
        if (balance_factor(n->right) > 0) n->right = rotate_right(n->right); // RL case
        return rotate_left(n);                                               // RR case
    }
    return n;
}

static SymNode *new_node(const char *key, void *value) {
    SymNode *n = xmalloc(sizeof(SymNode));
    const size_t len = strlen(key);
    n->key = xmalloc(len + 1);
    memcpy(n->key, key, len + 1);
    n->value = value;
    n->height = 1;
    n->left = n->right = NULL;
    return n;
}

static SymNode *insert(SymNode *n, const char *key, void *value, bool *inserted) {
    if (n == NULL) {
        *inserted = true;
        return new_node(key, value);
    }
    const int cmp = strcmp(key, n->key);
    if (cmp == 0) {
        *inserted = false;
        return n;
    }
    if (cmp < 0) n->left = insert(n->left, key, value, inserted);
    else n->right = insert(n->right, key, value, inserted);
    return *inserted ? rebalance(n) : n;
}

void symtable_init(SymTable *t) {
    t->root = NULL;
    t->count = 0;
}

bool symtable_insert(SymTable *t, const char *key, void *value) {
    bool inserted = false;
    t->root = insert(t->root, key, value, &inserted);
    if (inserted) t->count++;
    return inserted;
}

bool symtable_lookup(const SymTable *t, const char *key, void **value) {
    const SymNode *n = t->root;
    while (n != NULL) {
        const int cmp = strcmp(key, n->key);
        if (cmp == 0) {
            if (value) *value = n->value;
            return true;
        }
        n = cmp < 0 ? n->left : n->right;
    }
    return false;
}

static void foreach(const SymNode *n, void (*fn)(const char *, void *, void *), void *ctx) {
    if (n == NULL) return;
    foreach(n->left, fn, ctx);
    fn(n->key, n->value, ctx);
    foreach(n->right, fn, ctx);
}

void symtable_foreach(const SymTable *t, void (*fn)(const char *, void *, void *), void *ctx) {
    foreach(t->root, fn, ctx);
}

static void free_nodes(SymNode *n, void (*free_value)(void *)) {
    if (n == NULL) return;
    free_nodes(n->left, free_value);
    free_nodes(n->right, free_value);
    if (free_value) free_value(n->value);
    free(n->key);
    free(n);
}

void symtable_free(SymTable *t, void (*free_value)(void *)) {
    free_nodes(t->root, free_value);
    symtable_init(t);
}
