/*
 * Implementace překladače imperativního jazyka IFJ25
 * Abstract syntax tree.
 * Autoři: TODO (login, jméno)
 */

#include "ast.h"

#include <stdlib.h>
#include <string.h>

#include "error.h"

Expr *expr_new(ExprKind kind, int line, int col) {
    Expr *e = xmalloc(sizeof(Expr));
    memset(e, 0, sizeof(Expr));
    e->kind = kind;
    e->line = line;
    e->col = col;
    return e;
}

Stmt *stmt_new(StmtKind kind, int line, int col) {
    Stmt *s = xmalloc(sizeof(Stmt));
    memset(s, 0, sizeof(Stmt));
    s->kind = kind;
    s->line = line;
    s->col = col;
    return s;
}

Func *func_new(FuncKind kind, char *name, int line, int col) {
    Func *f = xmalloc(sizeof(Func));
    memset(f, 0, sizeof(Func));
    f->kind = kind;
    f->name = name;
    f->line = line;
    f->col = col;
    return f;
}

Program *program_new(void) {
    Program *p = xmalloc(sizeof(Program));
    memset(p, 0, sizeof(Program));
    symtable_init(&p->globals);
    return p;
}

void expr_free(Expr *e) {
    if (e == NULL) return;
    free(e->str);
    expr_free(e->left);
    expr_free(e->right);
    for (size_t i = 0; i < e->args.count; i++) expr_free(e->args.items[i]);
    vec_free(&e->args);
    free(e);
}

void stmt_free(Stmt *s) {
    if (s == NULL) return;
    free(s->name);
    expr_free(s->expr);
    stmt_free(s->then_block);
    stmt_free(s->else_block);
    for (size_t i = 0; i < s->stmts.count; i++) stmt_free(s->stmts.items[i]);
    vec_free(&s->stmts);
    free(s);
}

static void free_strings(Vec *v) {
    for (size_t i = 0; i < v->count; i++) free(v->items[i]);
    vec_free(v);
}

void program_free(Program *p) {
    if (p == NULL) return;
    for (size_t i = 0; i < p->funcs.count; i++) {
        Func *f = p->funcs.items[i];
        free(f->name);
        free_strings(&f->params);
        free_strings(&f->locals);
        stmt_free(f->body);
        free(f);
    }
    vec_free(&p->funcs);
    symtable_free(&p->globals, NULL);
    free(p);
}
