/*
 * Implementace překladače imperativního jazyka IFJ25
 * Abstract syntax tree.
 * Autoři: TODO (login, jméno)
 */

#ifndef IFJ25_AST_H
#define IFJ25_AST_H

#include <stdbool.h>
#include <stddef.h>

#include "symtable.h"
#include "token.h"
#include "vec.h"

/* ---------- expressions ---------- */

typedef enum {
    EX_NUM,     // num
    EX_STR,     // str, len
    EX_NULL,
    EX_TYPE,    // type: right operand of `is`
    EX_NAME,    // str = identifier; resolved by semantic analysis (local variable or getter)
    EX_GLOBAL,  // str = __name
    EX_BINARY,  // op, left, right   (op == TK_KW_IS for the type test)
    EX_CALL,    // str = function name, builtin, args (Vec of Expr*)
} ExprKind;

typedef enum { TYPE_NUM, TYPE_STRING, TYPE_NULL } TypeName;

typedef enum {
    NAME_UNRESOLVED,
    NAME_LOCAL,   // local variable or parameter, cg = name in the IFJcode25 frame
    NAME_GETTER,  // static getter call
    NAME_SETTER,  // assignment target that is a static setter
    NAME_GLOBAL,  // global variable (assignment target)
} NameKind;

typedef struct Expr {
    ExprKind kind;
    int line, col;
    double num;
    char *str;
    size_t len;
    TypeName type;
    TokenKind op;
    struct Expr *left, *right;
    bool builtin;
    Vec args;
    NameKind name_kind;   // EX_NAME after semantic analysis
    const char *cg;       // NAME_LOCAL: frame variable name (owned by the function's `locals`)
} Expr;

/* ---------- statements ---------- */

typedef enum {
    ST_VAR,     // name, expr = initializer or NULL
    ST_ASSIGN,  // name, global, expr
    ST_EXPR,    // expr = function call whose result is discarded
    ST_IF,      // expr = condition, then_block, else_block (NULL without else)
    ST_WHILE,   // expr = condition, then_block = body
    ST_RETURN,  // expr or NULL
    ST_BLOCK,   // stmts
} StmtKind;

typedef struct Stmt {
    StmtKind kind;
    int line, col;
    char *name;
    bool global;
    Expr *expr;
    struct Stmt *then_block, *else_block;
    Vec stmts;
    NameKind target_kind; // ST_ASSIGN after semantic analysis
    const char *cg;       // ST_VAR, ST_ASSIGN to a local: frame variable name
} Stmt;

/* ---------- functions, program ---------- */

typedef enum { FN_FUNCTION, FN_GETTER, FN_SETTER } FuncKind;

typedef struct {
    FuncKind kind;
    char *name;
    int line, col;
    Vec params;   // char* (setter: exactly one)
    Stmt *body;   // ST_BLOCK
    Vec locals;   // char*: frame names of all local variables, filled by semantic analysis
} Func;

typedef struct {
    Vec funcs;          // Func*
    SymTable globals;   // names of all global variables (__x) used anywhere
} Program;

Expr *expr_new(ExprKind kind, int line, int col);
Stmt *stmt_new(StmtKind kind, int line, int col);
Func *func_new(FuncKind kind, char *name, int line, int col);
Program *program_new(void);

void expr_free(Expr *e);
void stmt_free(Stmt *s);
void program_free(Program *p);

#endif
