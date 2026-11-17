/*
 * Implementace překladače imperativního jazyka IFJ25
 * Semantic analysis.
 * Autoři: TODO (login, jméno)
 *
 * Pass 1 enters all function headers into the global symbol table, so a
 * function can be called before its definition. Pass 2 walks every body with a
 * stack of symbol tables (one per block) and checks names, argument counts and
 * the types that are known at compile time (literals).
 *
 * Keys in the function table:
 *   "f:name/arity"  function          "g:name"  getter
 *   "s:name"        setter            "n:name"  some function with this name exists
 */

#include "semantic.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "symtable.h"

static const Builtin BUILTINS[] = {
    {"read_str", 0, {0}},
    {"read_num", 0, {0}},
    {"write", 1, {ARG_ANY}},
    {"floor", 1, {ARG_NUM}},
    {"str", 1, {ARG_ANY}},
    {"length", 1, {ARG_STRING}},
    {"substring", 3, {ARG_STRING, ARG_INT, ARG_INT}},
    {"strcmp", 2, {ARG_STRING, ARG_STRING}},
    {"ord", 2, {ARG_STRING, ARG_INT}},
    {"chr", 1, {ARG_INT}},
};

const Builtin *builtin_find(const char *name) {
    for (size_t i = 0; i < sizeof BUILTINS / sizeof BUILTINS[0]; i++) {
        if (strcmp(BUILTINS[i].name, name) == 0) return &BUILTINS[i];
    }
    return NULL;
}

// Statically known type of an expression.
typedef enum { T_ANY, T_NUM, T_STRING, T_NULL, T_BOOL } SType;

typedef struct {
    Program *program;
    SymTable funcs;
    Func *func;        // function being checked
    Vec scopes;        // SymTable*, innermost last; value = frame name (char*, owned by func->locals)
    int next_local;    // counter for unique frame names within the function
} Ctx;

static char *format(const char *fmt, const char *s, long n) {
    const int len = snprintf(NULL, 0, fmt, s, n);
    char *out = xmalloc((size_t) len + 1);
    snprintf(out, (size_t) len + 1, fmt, s, n);
    return out;
}

/* ---------- pass 1: function headers ---------- */

static void declare_functions(Ctx *c) {
    static int marker;
    for (size_t i = 0; i < c->program->funcs.count; i++) {
        Func *f = c->program->funcs.items[i];
        char *key;
        switch (f->kind) {
            case FN_FUNCTION: key = format("f:%s/%ld", f->name, (long) f->params.count); break;
            case FN_GETTER: key = format("g:%s", f->name, 0); break;
            default: key = format("s:%s", f->name, 0); break;
        }
        if (!symtable_insert(&c->funcs, key, f))
            fatal(ERR_SEM_REDEFINED, f->line, f->col, "redefinition of '%s'", f->name);
        free(key);
        if (f->kind == FN_FUNCTION) {
            key = format("n:%s", f->name, 0);
            symtable_insert(&c->funcs, key, &marker);
            free(key);
        }
    }
    if (!symtable_lookup(&c->funcs, "f:main/0", NULL)) fatal(ERR_SEM_UNDEFINED, 0, 0, "missing function main()");
}

static bool function_exists(const Ctx *c, char kind, const char *name, long arity) {
    char *key = kind == 'f' ? format("f:%s/%ld", name, arity) : format(kind == 'g' ? "g:%s" : kind == 's' ? "s:%s" : "n:%s", name, 0);
    const bool found = symtable_lookup(&c->funcs, key, NULL);
    free(key);
    return found;
}

/* ---------- scopes ---------- */

static void push_scope(Ctx *c) {
    SymTable *t = xmalloc(sizeof(SymTable));
    symtable_init(t);
    vec_push(&c->scopes, t);
}

static void pop_scope(Ctx *c) {
    SymTable *t = c->scopes.items[--c->scopes.count];
    symtable_free(t, NULL);
    free(t);
}

// Declares a local in the innermost scope; returns its frame name.
static const char *declare_local(Ctx *c, const char *name, const char *frame_name, int line, int col) {
    SymTable *scope = c->scopes.items[c->scopes.count - 1];
    char *cg = frame_name ? format("%s", frame_name, 0) : format("%s$%ld", name, c->next_local++);
    if (!symtable_insert(scope, name, cg))
        fatal(ERR_SEM_REDEFINED, line, col, "redefinition of variable '%s'", name);
    vec_push(&c->func->locals, cg);
    return cg;
}

static const char *find_local(const Ctx *c, const char *name) {
    for (size_t i = c->scopes.count; i-- > 0;) {
        void *cg;
        if (symtable_lookup(c->scopes.items[i], name, &cg)) return cg;
    }
    return NULL;
}

static void note_global(Ctx *c, const char *name) {
    symtable_insert(&c->program->globals, name, NULL);
}

/* ---------- expressions ---------- */

static bool num_compatible(SType t) { return t == T_ANY || t == T_NUM; }

static _Noreturn void type_error(const Expr *e, const char *msg) {
    fatal(ERR_SEM_TYPES, e->line, e->col, "%s", msg);
}

static SType check_expr(Ctx *c, Expr *e);

static void check_call(Ctx *c, Expr *call) {
    const long argc = (long) call->args.count;
    for (size_t i = 0; i < call->args.count; i++) check_expr(c, call->args.items[i]);

    if (!call->builtin) {
        if (function_exists(c, 'f', call->str, argc)) return;
        if (function_exists(c, 'n', call->str, 0))
            fatal(ERR_SEM_ARGS, call->line, call->col, "no overload of '%s' takes %ld arguments", call->str, argc);
        fatal(ERR_SEM_UNDEFINED, call->line, call->col, "undefined function '%s'", call->str);
    }

    const Builtin *b = builtin_find(call->str);
    if (b == NULL) fatal(ERR_SEM_UNDEFINED, call->line, call->col, "undefined builtin 'Ifj.%s'", call->str);
    if (b->arity != argc)
        fatal(ERR_SEM_ARGS, call->line, call->col, "'Ifj.%s' takes %d arguments, got %ld", b->name, b->arity, argc);

    // literal arguments of a wrong type are detected at compile time
    for (int i = 0; i < b->arity; i++) {
        const Expr *arg = call->args.items[i];
        const bool is_num = arg->kind == EX_NUM;
        const bool is_literal = is_num || arg->kind == EX_STR || arg->kind == EX_NULL;
        if (!is_literal || b->args[i] == ARG_ANY) continue;
        const bool want_string = b->args[i] == ARG_STRING;
        if (want_string ? arg->kind != EX_STR : !is_num)
            fatal(ERR_SEM_ARGS, arg->line, arg->col, "wrong argument type for 'Ifj.%s'", b->name);
        if (b->args[i] == ARG_INT && floor(arg->num) != arg->num)
            fatal(ERR_SEM_TYPES, arg->line, arg->col, "'Ifj.%s' expects an integer", b->name);
    }
}

static SType check_binary(Ctx *c, Expr *e) {
    const SType l = check_expr(c, e->left);
    if (e->op == TK_KW_IS) return T_BOOL; // right operand is a type name
    const SType r = check_expr(c, e->right);

    switch (e->op) {
        case TK_PLUS:
            if (l == T_NULL || r == T_NULL || l == T_BOOL || r == T_BOOL) type_error(e, "invalid operand of '+'");
            if (l != T_ANY && r != T_ANY && l != r) type_error(e, "'+' needs two numbers or two strings");
            return l != T_ANY ? l : r;
        case TK_MINUS:
        case TK_DIV:
            if (!num_compatible(l) || !num_compatible(r)) type_error(e, "arithmetic operator needs numbers");
            return T_NUM;
        case TK_MUL:
            if (!num_compatible(r)) type_error(e, "right operand of '*' must be a number");
            if (l != T_ANY && l != T_NUM && l != T_STRING) type_error(e, "invalid left operand of '*'");
            if (l == T_STRING && e->right->kind == EX_NUM && floor(e->right->num) != e->right->num)
                type_error(e, "string can only be repeated an integer number of times");
            return l;
        case TK_LT: case TK_GT: case TK_LE: case TK_GE:
            if (!num_compatible(l) || !num_compatible(r)) type_error(e, "relational operator needs numbers");
            return T_BOOL;
        default: // == !=
            return T_BOOL;
    }
}

static SType check_expr(Ctx *c, Expr *e) {
    switch (e->kind) {
        case EX_NUM: return T_NUM;
        case EX_STR: return T_STRING;
        case EX_NULL: return T_NULL;
        case EX_TYPE: return T_ANY;
        case EX_GLOBAL:
            note_global(c, e->str);
            return T_ANY;
        case EX_NAME:
            if ((e->cg = find_local(c, e->str)) != NULL) {
                e->name_kind = NAME_LOCAL;
            } else if (function_exists(c, 'g', e->str, 0)) {
                e->name_kind = NAME_GETTER;
            } else {
                fatal(ERR_SEM_UNDEFINED, e->line, e->col, "undefined variable '%s'", e->str);
            }
            return T_ANY;
        case EX_CALL:
            check_call(c, e);
            return T_ANY;
        case EX_BINARY:
            return check_binary(c, e);
    }
    return T_ANY;
}

/* ---------- statements ---------- */

static void check_stmt(Ctx *c, Stmt *s);

static void check_block(Ctx *c, Stmt *block, bool new_scope) {
    if (new_scope) push_scope(c);
    for (size_t i = 0; i < block->stmts.count; i++) check_stmt(c, block->stmts.items[i]);
    if (new_scope) pop_scope(c);
}

static void check_stmt(Ctx *c, Stmt *s) {
    switch (s->kind) {
        case ST_VAR:
            if (s->expr) check_expr(c, s->expr); // initializer sees the outer variable of the same name
            s->cg = declare_local(c, s->name, NULL, s->line, s->col);
            break;
        case ST_ASSIGN:
            check_expr(c, s->expr);
            if (s->global) {
                s->target_kind = NAME_GLOBAL;
                note_global(c, s->name);
            } else if ((s->cg = find_local(c, s->name)) != NULL) {
                s->target_kind = NAME_LOCAL;
            } else if (function_exists(c, 's', s->name, 0)) {
                s->target_kind = NAME_SETTER;
            } else {
                fatal(ERR_SEM_UNDEFINED, s->line, s->col, "undefined variable '%s'", s->name);
            }
            break;
        case ST_EXPR:
        case ST_RETURN:
            if (s->expr) check_expr(c, s->expr);
            break;
        case ST_IF:
            check_expr(c, s->expr);
            check_block(c, s->then_block, true);
            if (s->else_block) check_block(c, s->else_block, true);
            break;
        case ST_WHILE:
            check_expr(c, s->expr);
            check_block(c, s->then_block, true);
            break;
        case ST_BLOCK:
            check_block(c, s, true);
            break;
    }
}

/* ---------- pass 2: bodies ---------- */

static void check_function(Ctx *c, Func *f) {
    c->func = f;
    c->next_local = 0;
    // Parameters live in the same scope as the top level of the body,
    // so `var x` redefining parameter x is an error.
    push_scope(c);
    for (size_t i = 0; i < f->params.count; i++) {
        char frame_name[32];
        snprintf(frame_name, sizeof frame_name, "$p%zu", i); // defined by the caller (see codegen); cannot clash with name$N
        declare_local(c, f->params.items[i], frame_name, f->line, f->col);
    }
    check_block(c, f->body, false);
    pop_scope(c);
}

void analyse_program(Program *program) {
    Ctx c = {.program = program};
    symtable_init(&c.funcs);
    declare_functions(&c);
    for (size_t i = 0; i < program->funcs.count; i++) check_function(&c, program->funcs.items[i]);
    symtable_free(&c.funcs, NULL);
    vec_free(&c.scopes);
}
