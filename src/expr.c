/*
 * Implementace překladače imperativního jazyka IFJ25
 * Expression parsing: operator precedence analysis (see docs/grammar.md, section 5).
 * Autoři: TODO (login, jméno)
 */

#include "expr.h"

#include <stdlib.h>

#include "error.h"

// Terminal classes = rows/columns of the precedence table.
typedef enum { P_MUL, P_ADD, P_REL, P_IS, P_EQ, P_LPAR, P_RPAR, P_TERM, P_END, P_COUNT } PClass;

// '<' shift with handle mark, '=' shift, '>' reduce, ' ' error
static const char TABLE[P_COUNT][P_COUNT] = {
    /*            M    A    R    I    E    (    )    i    $  */
    [P_MUL]  = {'>', '>', '>', '>', '>', '<', '>', '<', '>'},
    [P_ADD]  = {'<', '>', '>', '>', '>', '<', '>', '<', '>'},
    [P_REL]  = {'<', '<', '>', '>', '>', '<', '>', '<', '>'},
    [P_IS]   = {'<', '<', '<', '>', '>', '<', '>', '<', '>'},
    [P_EQ]   = {'<', '<', '<', '<', '>', '<', '>', '<', '>'},
    [P_LPAR] = {'<', '<', '<', '<', '<', '<', '=', '<', ' '},
    [P_RPAR] = {'>', '>', '>', '>', '>', ' ', '>', ' ', '>'},
    [P_TERM] = {'>', '>', '>', '>', '>', ' ', '>', ' ', '>'},
    [P_END]  = {'<', '<', '<', '<', '<', '<', ' ', '<', ' '},
};

typedef enum { SYM_TERMINAL, SYM_EXPR, SYM_HANDLE } SymType;

typedef struct {
    SymType type;
    PClass cls;    // SYM_TERMINAL
    TokenKind op;  // SYM_TERMINAL: operator token
    Expr *expr;    // SYM_TERMINAL of class P_TERM (the leaf) or SYM_EXPR
    int line, col;
} Sym;

typedef struct {
    Sym *items;
    size_t count, capacity;
} Stack;

static void push(Stack *s, Sym sym) {
    if (s->count == s->capacity) {
        s->capacity = s->capacity ? s->capacity * 2 : 16;
        s->items = xrealloc(s->items, s->capacity * sizeof(Sym));
    }
    s->items[s->count++] = sym;
}

static size_t top_terminal(const Stack *s) {
    size_t i = s->count;
    while (i-- > 0) {
        if (s->items[i].type == SYM_TERMINAL) return i;
    }
    return 0; // unreachable: the bottom is always the P_END terminal
}

static PClass classify(TokenKind kind) {
    switch (kind) {
        case TK_MUL: case TK_DIV: return P_MUL;
        case TK_PLUS: case TK_MINUS: return P_ADD;
        case TK_LT: case TK_GT: case TK_LE: case TK_GE: return P_REL;
        case TK_KW_IS: return P_IS;
        case TK_EQ: case TK_NE: return P_EQ;
        case TK_LPAREN: return P_LPAR;
        case TK_RPAREN: return P_RPAR;
        case TK_IDENT: case TK_GID: case TK_INT: case TK_FLOAT: case TK_STRING: case TK_KW_NULL:
        case TK_KW_TYPE_NUM: case TK_KW_TYPE_STRING: case TK_KW_TYPE_NULL:
            return P_TERM;
        default:
            return P_END;
    }
}

Expr *expr_leaf(Parser *p) {
    const Token *t = &p->cur;
    Expr *e;
    switch (t->kind) {
        case TK_IDENT:  e = expr_new(EX_NAME, t->line, t->col); e->str = parser_take_str(p); break;
        case TK_GID:    e = expr_new(EX_GLOBAL, t->line, t->col); e->str = parser_take_str(p); break;
        case TK_INT:    e = expr_new(EX_NUM, t->line, t->col); e->num = (double) t->i; break;
        case TK_FLOAT:  e = expr_new(EX_NUM, t->line, t->col); e->num = t->f; break;
        case TK_STRING:
            e = expr_new(EX_STR, t->line, t->col);
            e->len = t->len;
            e->str = parser_take_str(p);
            break;
        case TK_KW_NULL: e = expr_new(EX_NULL, t->line, t->col); break;
        case TK_KW_TYPE_NUM: e = expr_new(EX_TYPE, t->line, t->col); e->type = TYPE_NUM; break;
        case TK_KW_TYPE_STRING: e = expr_new(EX_TYPE, t->line, t->col); e->type = TYPE_STRING; break;
        default: e = expr_new(EX_TYPE, t->line, t->col); e->type = TYPE_NULL; break;
    }
    parser_next(p);
    return e;
}

static _Noreturn void type_name_error(const Expr *e) {
    fatal(ERR_SYNTAX, e->line, e->col, "type name can only be the right operand of 'is'");
}

// Reduces the handle on top of the stack (everything above the last handle mark).
static void reduce(Parser *p, Stack *s) {
    size_t h = s->count - 1;
    while (h > 0 && s->items[h].type != SYM_HANDLE) h--;
    if (s->items[h].type != SYM_HANDLE) parser_error(p, "valid expression");

    Sym *rhs = &s->items[h + 1];
    const size_t n = s->count - h - 1;
    Sym result = {.type = SYM_EXPR};

    if (n == 1 && rhs[0].type == SYM_TERMINAL && rhs[0].cls == P_TERM) {
        result.expr = rhs[0].expr;                                   // E -> i
    } else if (n == 3 && rhs[0].type == SYM_TERMINAL && rhs[0].cls == P_LPAR && rhs[1].type == SYM_EXPR
               && rhs[2].type == SYM_TERMINAL && rhs[2].cls == P_RPAR) {
        result.expr = rhs[1].expr;                                   // E -> ( E )
    } else if (n == 3 && rhs[0].type == SYM_EXPR && rhs[1].type == SYM_TERMINAL && rhs[1].cls <= P_EQ
               && rhs[2].type == SYM_EXPR) {
        Expr *left = rhs[0].expr, *right = rhs[2].expr;              // E -> E op E
        if (left->kind == EX_TYPE) type_name_error(left);
        if (rhs[1].op == TK_KW_IS) {
            if (right->kind != EX_TYPE)
                fatal(ERR_SYNTAX, right->line, right->col, "expected Num, String or Null after 'is'");
        } else if (right->kind == EX_TYPE) {
            type_name_error(right);
        }
        Expr *e = expr_new(EX_BINARY, rhs[1].line, rhs[1].col);
        e->op = rhs[1].op;
        e->left = left;
        e->right = right;
        result.expr = e;
    } else {
        parser_error(p, "valid expression");
    }

    s->count = h; // pop the handle and its symbols
    push(s, result);
}

Expr *expr_parse(Parser *p, Expr *seed) {
    Stack stack = {0};
    push(&stack, (Sym){.type = SYM_TERMINAL, .cls = P_END});
    int depth = 0; // unmatched '(' inside this expression

    for (;;) {
        // Current input symbol: the seed first, then tokens.
        PClass in = seed ? P_TERM : classify(p->cur.kind);
        if (in == P_RPAR && depth == 0) in = P_END;

        const size_t top = top_terminal(&stack);
        const PClass a = stack.items[top].cls;

        if (a == P_END && in == P_END) {
            if (stack.count != 2 || stack.items[1].type != SYM_EXPR) parser_error(p, "expression");
            Expr *e = stack.items[1].expr;
            free(stack.items);
            if (e->kind == EX_TYPE) type_name_error(e);
            return e;
        }

        const char rel = TABLE[a][in];
        if (rel == '<' || rel == '=') {
            Sym sym = {.type = SYM_TERMINAL, .cls = in, .op = p->cur.kind, .line = p->cur.line, .col = p->cur.col};
            if (seed) {
                sym.expr = seed;
                sym.line = seed->line;
                sym.col = seed->col;
                seed = NULL;
            } else if (in == P_TERM) {
                sym.expr = expr_leaf(p);
            } else {
                if (in == P_LPAR) depth++;
                if (in == P_RPAR) depth--;
                parser_next(p);
            }
            if (rel == '<') {
                // insert the handle mark right above the topmost terminal
                push(&stack, (Sym){0});
                for (size_t i = stack.count - 1; i > top + 1; i--) stack.items[i] = stack.items[i - 1];
                stack.items[top + 1] = (Sym){.type = SYM_HANDLE};
            }
            push(&stack, sym);
        } else if (rel == '>') {
            reduce(p, &stack);
        } else {
            parser_error(p, in == P_END ? "operand or operator" : "valid expression");
        }
    }
}
