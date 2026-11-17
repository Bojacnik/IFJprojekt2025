/*
 * Implementace překladače imperativního jazyka IFJ25
 * Syntax analysis: recursive descent (LL(1), see docs/grammar.md).
 * Autoři: TODO (login, jméno)
 *
 * Every nonterminal of the grammar has one function; the `switch` on the
 * lookahead token in each function corresponds to one row of the LL table.
 * Rule numbers refer to docs/grammar.md.
 */

#include "parser.h"

#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "expr.h"

/* ---------- helpers ---------- */

void parser_next(Parser *p) {
    token_free(&p->cur);
    p->cur = lexer_next(p->lexer);
}

char *parser_take_str(Parser *p) {
    char *s = p->cur.str;
    p->cur.str = NULL;
    return s;
}

void parser_error(const Parser *p, const char *expected) {
    fatal(ERR_SYNTAX, p->cur.line, p->cur.col, "expected %s, got %s", expected, TOKEN_NAMES[p->cur.kind]);
}

static void expect(Parser *p, TokenKind kind, const char *what) {
    if (p->cur.kind != kind) parser_error(p, what);
    parser_next(p);
}

static char *expect_ident(Parser *p) {
    if (p->cur.kind != TK_IDENT) parser_error(p, "identifier");
    char *name = parser_take_str(p);
    parser_next(p);
    return name;
}

/* ---------- forward declarations ---------- */

static Stmt *parse_block(Parser *p);
static Expr *parse_rhs(Parser *p);

/* ---------- calls and terms ---------- */

// TERM -> id | gid | int | float | string | null          (rules 40-45)
static Expr *parse_term(Parser *p) {
    switch (p->cur.kind) {
        case TK_IDENT: case TK_GID: case TK_INT: case TK_FLOAT: case TK_STRING: case TK_KW_NULL:
            return expr_leaf(p);
        default:
            parser_error(p, "argument (identifier or literal)");
    }
}

// ( ARGS )   ARGS -> TERM ARGS_TAIL | ε,  ARGS_TAIL -> , TERM ARGS_TAIL | ε   (rules 36-39)
static void parse_args(Parser *p, Expr *call) {
    expect(p, TK_LPAREN, "'('");
    if (p->cur.kind != TK_RPAREN) {
        vec_push(&call->args, parse_term(p));
        while (p->cur.kind == TK_COMMA) {
            parser_next(p);
            vec_push(&call->args, parse_term(p));
        }
    }
    expect(p, TK_RPAREN, "')'");
}

// call of a user function; the name has already been read
static Expr *parse_user_call(Parser *p, char *name, int line, int col) {
    Expr *call = expr_new(EX_CALL, line, col);
    call->str = name;
    parse_args(p, call);
    return call;
}

// BUILTIN_CALL -> Ifj . id ( ARGS )                     (rule 33)
static Expr *parse_builtin_call(Parser *p) {
    Expr *call = expr_new(EX_CALL, p->cur.line, p->cur.col);
    call->builtin = true;
    expect(p, TK_KW_IFJ, "'Ifj'");
    expect(p, TK_DOT, "'.'");
    call->str = expect_ident(p);
    parse_args(p, call);
    return call;
}

/* ---------- right-hand sides ---------- */

// RHS -> id RHS_ID | BUILTIN_CALL | EXPR⁻                 (rules 28-30)
// RHS_ID -> ( ARGS ) | EXPR_REST                          (rules 31-32)
static Expr *parse_rhs(Parser *p) {
    switch (p->cur.kind) {
        case TK_IDENT: {
            const int line = p->cur.line, col = p->cur.col;
            char *name = parser_take_str(p);
            parser_next(p);
            if (p->cur.kind == TK_LPAREN) return parse_user_call(p, name, line, col);
            Expr *seed = expr_new(EX_NAME, line, col);
            seed->str = name;
            return expr_parse(p, seed);
        }
        case TK_KW_IFJ:
            return parse_builtin_call(p);
        case TK_GID: case TK_INT: case TK_FLOAT: case TK_STRING: case TK_KW_NULL: case TK_LPAREN:
            return expr_parse(p, NULL);
        default:
            parser_error(p, "expression or function call");
    }
}

/* ---------- statements ---------- */

static Stmt *parse_stmt(Parser *p) {
    const int line = p->cur.line, col = p->cur.col;
    Stmt *s;

    switch (p->cur.kind) {
        case TK_KW_VAR: // STMT -> var id VAR_INIT,  VAR_INIT -> = RHS | ε   (rules 16, 24, 25)
            parser_next(p);
            s = stmt_new(ST_VAR, line, col);
            s->name = expect_ident(p);
            if (p->cur.kind == TK_ASSIGN) {
                parser_next(p);
                s->expr = parse_rhs(p);
            } else if (p->cur.kind != TK_EOL) {
                parser_error(p, "'=' or end of line");
            }
            return s;

        case TK_IDENT: { // STMT -> id ID_TAIL,  ID_TAIL -> = RHS | ( ARGS )   (rules 17, 26, 27)
            char *name = parser_take_str(p);
            parser_next(p);
            if (p->cur.kind == TK_ASSIGN) {
                parser_next(p);
                s = stmt_new(ST_ASSIGN, line, col);
                s->name = name;
                s->expr = parse_rhs(p);
            } else if (p->cur.kind == TK_LPAREN) {
                s = stmt_new(ST_EXPR, line, col);
                s->expr = parse_user_call(p, name, line, col);
            } else {
                parser_error(p, "'=' or '('");
            }
            return s;
        }

        case TK_GID: // STMT -> gid = RHS   (rule 18)
            s = stmt_new(ST_ASSIGN, line, col);
            s->name = parser_take_str(p);
            s->global = true;
            parser_next(p);
            expect(p, TK_ASSIGN, "'='");
            s->expr = parse_rhs(p);
            return s;

        case TK_KW_IFJ: // STMT -> BUILTIN_CALL   (rule 19)
            s = stmt_new(ST_EXPR, line, col);
            s->expr = parse_builtin_call(p);
            return s;

        case TK_KW_IF: // STMT -> if ( EXPR ) BLOCK ELSE_PART,  ELSE_PART -> else BLOCK | ε   (rules 20, 46, 47)
            parser_next(p);
            s = stmt_new(ST_IF, line, col);
            expect(p, TK_LPAREN, "'('");
            s->expr = expr_parse(p, NULL);
            expect(p, TK_RPAREN, "')'");
            s->then_block = parse_block(p);
            if (p->cur.kind == TK_KW_ELSE) {
                parser_next(p);
                s->else_block = parse_block(p);
            } else if (p->cur.kind != TK_EOL) {
                parser_error(p, "'else' or end of line");
            }
            return s;

        case TK_KW_WHILE: // STMT -> while ( EXPR ) BLOCK   (rule 21)
            parser_next(p);
            s = stmt_new(ST_WHILE, line, col);
            expect(p, TK_LPAREN, "'('");
            s->expr = expr_parse(p, NULL);
            expect(p, TK_RPAREN, "')'");
            s->then_block = parse_block(p);
            return s;

        case TK_KW_RETURN: // STMT -> return RET_VAL,  RET_VAL -> RHS | ε   (rules 22, 34, 35)
            parser_next(p);
            s = stmt_new(ST_RETURN, line, col);
            if (p->cur.kind != TK_EOL) s->expr = parse_rhs(p);
            return s;

        case TK_LBRACE: // STMT -> BLOCK   (rule 23)
            return parse_block(p);

        default:
            parser_error(p, "statement");
    }
}

// BLOCK -> { EOL STMTS },  STMTS -> STMT EOL STMTS | ε   (rules 13-15)
static Stmt *parse_block(Parser *p) {
    Stmt *block = stmt_new(ST_BLOCK, p->cur.line, p->cur.col);
    expect(p, TK_LBRACE, "'{'");
    expect(p, TK_EOL, "end of line after '{'");
    while (p->cur.kind != TK_RBRACE) {
        vec_push(&block->stmts, parse_stmt(p));
        expect(p, TK_EOL, "end of line after statement");
    }
    parser_next(p);
    return block;
}

/* ---------- functions and program ---------- */

// static id FUNC_KIND   (rules 4, 6-12)
static Func *parse_func(Parser *p) {
    expect(p, TK_KW_STATIC, "'static'");
    const int line = p->cur.line, col = p->cur.col;
    char *name = expect_ident(p);
    Func *f;

    switch (p->cur.kind) {
        case TK_LPAREN: // function: ( PARAMS ) BLOCK
            f = func_new(FN_FUNCTION, name, line, col);
            parser_next(p);
            if (p->cur.kind != TK_RPAREN) {
                vec_push(&f->params, expect_ident(p));
                while (p->cur.kind == TK_COMMA) {
                    parser_next(p);
                    vec_push(&f->params, expect_ident(p));
                }
            }
            expect(p, TK_RPAREN, "')'");
            break;
        case TK_LBRACE: // getter: BLOCK
            f = func_new(FN_GETTER, name, line, col);
            break;
        case TK_ASSIGN: // setter: = ( id ) BLOCK
            f = func_new(FN_SETTER, name, line, col);
            parser_next(p);
            expect(p, TK_LPAREN, "'('");
            vec_push(&f->params, expect_ident(p));
            expect(p, TK_RPAREN, "')'");
            break;
        default:
            parser_error(p, "'(', '{' or '='");
    }
    f->body = parse_block(p);
    return f;
}

// PROGRAM -> import string for Ifj EOL class id { EOL FUNCS } OPT_EOL eof   (rules 1-5)
Program *parse_program(Lexer *lexer) {
    Parser parser = {.lexer = lexer};
    Parser *p = &parser;
    p->cur = lexer_next(lexer);

    expect(p, TK_KW_IMPORT, "'import'");
    if (p->cur.kind != TK_STRING || strcmp(p->cur.str, "ifj25") != 0) parser_error(p, "\"ifj25\"");
    parser_next(p);
    expect(p, TK_KW_FOR, "'for'");
    expect(p, TK_KW_IFJ, "'Ifj'");
    expect(p, TK_EOL, "end of line after the prolog");

    expect(p, TK_KW_CLASS, "'class'");
    if (p->cur.kind != TK_IDENT || strcmp(p->cur.str, "Program") != 0) parser_error(p, "'Program'");
    parser_next(p);
    expect(p, TK_LBRACE, "'{'");
    expect(p, TK_EOL, "end of line after '{'");

    Program *program = program_new();
    while (p->cur.kind == TK_KW_STATIC) {
        vec_push(&program->funcs, parse_func(p));
        expect(p, TK_EOL, "end of line after function definition");
    }
    expect(p, TK_RBRACE, "'static' or '}'");

    if (p->cur.kind == TK_EOL) parser_next(p);
    if (p->cur.kind != TK_EOF) parser_error(p, "end of file");
    token_free(&p->cur);
    return program;
}
