/*
 * Implementace překladače imperativního jazyka IFJ25
 * Code generation: IFJcode25.
 * Autoři: TODO (login, jméno)
 *
 * Design:
 *  - Every Num is an IFJcode25 float (spec §5: Num corresponds to float).
 *    Ifj.write / Ifj.str print integral values as integers.
 *  - Expressions are evaluated on the data stack. Operators are small
 *    subroutines ($$add, $$eq, ...) that check the operand types at run time
 *    (exit 26) and push the result. They use GF@$$* temporaries and never
 *    call user code, so they need no frame.
 *  - Calling convention: arguments are evaluated onto the data stack, then
 *    CREATEFRAME, DEFVAR TF@$p0..n, POPS (last argument first), CALL.
 *    The callee does PUSHFRAME, DEFVARs all its locals at the start (so a
 *    DEFVAR is never executed twice inside a loop), and returns its value on
 *    the data stack after POPFRAME.
 *  - Labels: $f$name$arity (function), $g$name (getter), $s$name (setter),
 *    $$... (runtime helpers), $L<n> (control flow).
 */

#include "codegen.h"

#include <string.h>

#include "semantic.h"

typedef struct {
    FILE *out;
    unsigned label;
} Gen;

#define EMIT(...) (fprintf(g->out, __VA_ARGS__), fputc('\n', g->out))

/* ---------- operands ---------- */

static void emit_string_const(Gen *g, const char *s, size_t len) {
    fputs("string@", g->out);
    for (size_t i = 0; i < len; i++) {
        const unsigned char ch = (unsigned char) s[i];
        if (ch <= 32 || ch == '#' || ch == '\\' || ch >= 127) fprintf(g->out, "\\%03u", ch);
        else fputc(ch, g->out);
    }
}

static void emit_push_num(Gen *g, double value) {
    fprintf(g->out, "PUSHS float@%a\n", value);
}

/* ---------- expressions ---------- */

static void gen_expr(Gen *g, const Expr *e);

static void gen_call(Gen *g, const Expr *call) {
    for (size_t i = 0; i < call->args.count; i++) gen_expr(g, call->args.items[i]);

    if (call->builtin) {
        EMIT("CALL $$b_%s", call->str);
        return;
    }
    EMIT("CREATEFRAME");
    for (size_t i = 0; i < call->args.count; i++) EMIT("DEFVAR TF@$p%zu", i);
    for (size_t i = call->args.count; i-- > 0;) EMIT("POPS TF@$p%zu", i);
    EMIT("CALL $f$%s$%zu", call->str, call->args.count);
}

static const char *op_helper(TokenKind op) {
    switch (op) {
        case TK_PLUS: return "$$add";
        case TK_MINUS: return "$$sub";
        case TK_MUL: return "$$mul";
        case TK_DIV: return "$$div";
        case TK_LT: return "$$lt";
        case TK_GT: return "$$gt";
        case TK_LE: return "$$le";
        case TK_GE: return "$$ge";
        case TK_EQ: return "$$eq";
        case TK_NE: return "$$ne";
        default: return "$$is";
    }
}

static void gen_expr(Gen *g, const Expr *e) {
    switch (e->kind) {
        case EX_NUM:
            emit_push_num(g, e->num);
            break;
        case EX_STR:
            fputs("PUSHS ", g->out);
            emit_string_const(g, e->str, e->len);
            fputc('\n', g->out);
            break;
        case EX_NULL:
            EMIT("PUSHS nil@nil");
            break;
        case EX_TYPE: // right operand of `is`: the IFJcode25 type name
            EMIT("PUSHS string@%s", e->type == TYPE_NUM ? "float" : e->type == TYPE_STRING ? "string" : "nil");
            break;
        case EX_GLOBAL:
            EMIT("PUSHS GF@%s", e->str);
            break;
        case EX_NAME:
            if (e->name_kind == NAME_LOCAL) {
                EMIT("PUSHS LF@%s", e->cg);
            } else { // getter
                EMIT("CREATEFRAME");
                EMIT("CALL $g$%s", e->str);
            }
            break;
        case EX_CALL:
            gen_call(g, e);
            break;
        case EX_BINARY:
            gen_expr(g, e->left);
            gen_expr(g, e->right);
            EMIT("CALL %s", op_helper(e->op));
            break;
    }
}

/* ---------- statements ---------- */

static void gen_block(Gen *g, const Stmt *block);

static void gen_condition_jump_if_false(Gen *g, const Expr *cond, unsigned label) {
    gen_expr(g, cond);
    EMIT("CALL $$truthy");
    EMIT("PUSHS bool@true");
    EMIT("JUMPIFNEQS $L%u", label);
}

static void gen_stmt(Gen *g, const Stmt *s) {
    switch (s->kind) {
        case ST_VAR:
            if (s->expr) {
                gen_expr(g, s->expr);
                EMIT("POPS LF@%s", s->cg);
            } else {
                EMIT("MOVE LF@%s nil@nil", s->cg);
            }
            break;

        case ST_ASSIGN:
            gen_expr(g, s->expr);
            switch (s->target_kind) {
                case NAME_LOCAL: EMIT("POPS LF@%s", s->cg); break;
                case NAME_GLOBAL: EMIT("POPS GF@%s", s->name); break;
                default: // setter
                    EMIT("CREATEFRAME");
                    EMIT("DEFVAR TF@$p0");
                    EMIT("POPS TF@$p0");
                    EMIT("CALL $s$%s", s->name);
                    EMIT("POPS GF@$$discard");
                    break;
            }
            break;

        case ST_EXPR:
            gen_expr(g, s->expr);
            EMIT("POPS GF@$$discard");
            break;

        case ST_IF: {
            const unsigned l_else = g->label++, l_end = g->label++;
            gen_condition_jump_if_false(g, s->expr, l_else);
            gen_block(g, s->then_block);
            EMIT("JUMP $L%u", l_end);
            EMIT("LABEL $L%u", l_else);
            if (s->else_block) gen_block(g, s->else_block);
            EMIT("LABEL $L%u", l_end);
            break;
        }

        case ST_WHILE: {
            const unsigned l_cond = g->label++, l_end = g->label++;
            EMIT("LABEL $L%u", l_cond);
            gen_condition_jump_if_false(g, s->expr, l_end);
            gen_block(g, s->then_block);
            EMIT("JUMP $L%u", l_cond);
            EMIT("LABEL $L%u", l_end);
            break;
        }

        case ST_RETURN:
            if (s->expr) gen_expr(g, s->expr);
            else EMIT("PUSHS nil@nil");
            EMIT("POPFRAME");
            EMIT("RETURN");
            break;

        case ST_BLOCK:
            gen_block(g, s);
            break;
    }
}

static void gen_block(Gen *g, const Stmt *block) {
    for (size_t i = 0; i < block->stmts.count; i++) gen_stmt(g, block->stmts.items[i]);
}

/* ---------- functions ---------- */

static void gen_function(Gen *g, const Func *f) {
    fputc('\n', g->out);
    switch (f->kind) {
        case FN_FUNCTION: EMIT("LABEL $f$%s$%zu", f->name, f->params.count); break;
        case FN_GETTER: EMIT("LABEL $g$%s", f->name); break;
        case FN_SETTER: EMIT("LABEL $s$%s", f->name); break;
    }
    EMIT("PUSHFRAME");
    // locals[0 .. params-1] are the parameters, already defined by the caller
    for (size_t i = f->params.count; i < f->locals.count; i++) EMIT("DEFVAR LF@%s", (char *) f->locals.items[i]);
    gen_block(g, f->body);
    EMIT("PUSHS nil@nil");
    EMIT("POPFRAME");
    EMIT("RETURN");
}

/* ---------- runtime helpers ---------- */

static const char *const TEMPS[] = {"a", "b", "ta", "tb", "t", "r", "n", "i", "j", "discard"};

static const char *const RUNTIME[] = {
    "\n# ----- runtime helpers -----\n",
    "LABEL $$err25\nEXIT int@25\n",
    "LABEL $$err26\nEXIT int@26\n",
    // pop b, then a, and their types
    "LABEL $$pop2\nPOPS GF@$$b\nPOPS GF@$$a\nTYPE GF@$$tb GF@$$b\nTYPE GF@$$ta GF@$$a\nRETURN\n",
    // both operands must be numbers
    "LABEL $$nums\nCALL $$pop2\n",
    "JUMPIFNEQ $$err26 GF@$$ta string@float\nJUMPIFNEQ $$err26 GF@$$tb string@float\nRETURN\n",
    // condition value -> bool (null is false, bool as is, anything else true)
    "LABEL $$truthy\nPOPS GF@$$a\nTYPE GF@$$ta GF@$$a\n",
    "JUMPIFEQ $$truthy$bool GF@$$ta string@bool\nJUMPIFEQ $$truthy$false GF@$$ta string@nil\n",
    "PUSHS bool@true\nRETURN\n",
    "LABEL $$truthy$bool\nPUSHS GF@$$a\nRETURN\n",
    "LABEL $$truthy$false\nPUSHS bool@false\nRETURN\n",
    // + : numbers or strings
    "LABEL $$add\nCALL $$pop2\nJUMPIFNEQ $$err26 GF@$$ta GF@$$tb\n",
    "JUMPIFEQ $$add$num GF@$$ta string@float\nJUMPIFEQ $$add$str GF@$$ta string@string\nJUMP $$err26\n",
    "LABEL $$add$num\nADD GF@$$r GF@$$a GF@$$b\nPUSHS GF@$$r\nRETURN\n",
    "LABEL $$add$str\nCONCAT GF@$$r GF@$$a GF@$$b\nPUSHS GF@$$r\nRETURN\n",
    "LABEL $$sub\nCALL $$nums\nSUB GF@$$r GF@$$a GF@$$b\nPUSHS GF@$$r\nRETURN\n",
    "LABEL $$div\nCALL $$nums\nDIV GF@$$r GF@$$a GF@$$b\nPUSHS GF@$$r\nRETURN\n",
    // * : numbers, or string * integer (repetition)
    "LABEL $$mul\nCALL $$pop2\nJUMPIFNEQ $$err26 GF@$$tb string@float\n",
    "JUMPIFEQ $$mul$num GF@$$ta string@float\nJUMPIFEQ $$mul$str GF@$$ta string@string\nJUMP $$err26\n",
    "LABEL $$mul$num\nMUL GF@$$r GF@$$a GF@$$b\nPUSHS GF@$$r\nRETURN\n",
    "LABEL $$mul$str\nISINT GF@$$t GF@$$b\nJUMPIFNEQ $$err26 GF@$$t bool@true\n",
    "FLOAT2INT GF@$$n GF@$$b\nMOVE GF@$$r string@\n",
    "LABEL $$mul$loop\nLT GF@$$t GF@$$n int@1\nJUMPIFEQ $$mul$done GF@$$t bool@true\n",
    "CONCAT GF@$$r GF@$$r GF@$$a\nSUB GF@$$n GF@$$n int@1\nJUMP $$mul$loop\n",
    "LABEL $$mul$done\nPUSHS GF@$$r\nRETURN\n",
    // relational operators on numbers
    "LABEL $$lt\nCALL $$nums\nLT GF@$$r GF@$$a GF@$$b\nPUSHS GF@$$r\nRETURN\n",
    "LABEL $$gt\nCALL $$nums\nGT GF@$$r GF@$$a GF@$$b\nPUSHS GF@$$r\nRETURN\n",
    "LABEL $$le\nCALL $$nums\nGT GF@$$r GF@$$a GF@$$b\nNOT GF@$$r GF@$$r\nPUSHS GF@$$r\nRETURN\n",
    "LABEL $$ge\nCALL $$nums\nLT GF@$$r GF@$$a GF@$$b\nNOT GF@$$r GF@$$r\nPUSHS GF@$$r\nRETURN\n",
    // == : different types are never equal
    "LABEL $$eq\nCALL $$pop2\nJUMPIFNEQ $$eq$false GF@$$ta GF@$$tb\n",
    "EQ GF@$$r GF@$$a GF@$$b\nPUSHS GF@$$r\nRETURN\n",
    "LABEL $$eq$false\nPUSHS bool@false\nRETURN\n",
    "LABEL $$ne\nCALL $$eq\nNOTS\nRETURN\n",
    // is : b is the IFJcode25 type name pushed by the compiler
    "LABEL $$is\nPOPS GF@$$b\nPOPS GF@$$a\nTYPE GF@$$ta GF@$$a\nEQ GF@$$r GF@$$ta GF@$$b\nPUSHS GF@$$r\nRETURN\n",
    // Ifj.write: integral numbers are printed as integers, null as "null"
    "LABEL $$b_write\nPOPS GF@$$a\nTYPE GF@$$ta GF@$$a\n",
    "JUMPIFEQ $$write$nil GF@$$ta string@nil\nJUMPIFNEQ $$write$plain GF@$$ta string@float\n",
    "ISINT GF@$$t GF@$$a\nJUMPIFNEQ $$write$plain GF@$$t bool@true\n",
    "FLOAT2INT GF@$$n GF@$$a\nWRITE GF@$$n\nPUSHS nil@nil\nRETURN\n",
    "LABEL $$write$plain\nWRITE GF@$$a\nPUSHS nil@nil\nRETURN\n",
    "LABEL $$write$nil\nWRITE string@null\nPUSHS nil@nil\nRETURN\n",
    "LABEL $$b_read_str\nREAD GF@$$r string\nPUSHS GF@$$r\nRETURN\n",
    "LABEL $$b_read_num\nREAD GF@$$r float\nPUSHS GF@$$r\nRETURN\n",
    "LABEL $$b_floor\nPOPS GF@$$a\nTYPE GF@$$ta GF@$$a\nJUMPIFNEQ $$err25 GF@$$ta string@float\n",
    "FLOAT2INT GF@$$n GF@$$a\nINT2FLOAT GF@$$r GF@$$n\nPUSHS GF@$$r\nRETURN\n",
    "LABEL $$b_str\nPOPS GF@$$a\nTYPE GF@$$ta GF@$$a\n",
    "JUMPIFEQ $$str$nil GF@$$ta string@nil\nJUMPIFEQ $$str$num GF@$$ta string@float\n",
    "JUMPIFEQ $$str$bool GF@$$ta string@bool\nPUSHS GF@$$a\nRETURN\n",
    "LABEL $$str$nil\nPUSHS string@null\nRETURN\n",
    "LABEL $$str$bool\nJUMPIFEQ $$str$true GF@$$a bool@true\nPUSHS string@false\nRETURN\n",
    "LABEL $$str$true\nPUSHS string@true\nRETURN\n",
    "LABEL $$str$num\nISINT GF@$$t GF@$$a\nJUMPIFNEQ $$str$frac GF@$$t bool@true\n",
    "FLOAT2INT GF@$$n GF@$$a\nINT2STR GF@$$r GF@$$n\nPUSHS GF@$$r\nRETURN\n",
    "LABEL $$str$frac\nFLOAT2STR GF@$$r GF@$$a\nPUSHS GF@$$r\nRETURN\n",
    "LABEL $$b_length\nPOPS GF@$$a\nTYPE GF@$$ta GF@$$a\nJUMPIFNEQ $$err25 GF@$$ta string@string\n",
    "STRLEN GF@$$n GF@$$a\nINT2FLOAT GF@$$r GF@$$n\nPUSHS GF@$$r\nRETURN\n",
    "LABEL $$b_strcmp\nCALL $$pop2\n",
    "JUMPIFNEQ $$err25 GF@$$ta string@string\nJUMPIFNEQ $$err25 GF@$$tb string@string\n",
    "LT GF@$$t GF@$$a GF@$$b\nJUMPIFEQ $$strcmp$less GF@$$t bool@true\n",
    "EQ GF@$$t GF@$$a GF@$$b\nJUMPIFEQ $$strcmp$eq GF@$$t bool@true\n",
    "PUSHS float@0x1p+0\nRETURN\n",
    "LABEL $$strcmp$less\nPUSHS float@-0x1p+0\nRETURN\n",
    "LABEL $$strcmp$eq\nPUSHS float@0x0p+0\nRETURN\n",
    "LABEL $$b_ord\nCALL $$pop2\n",
    "JUMPIFNEQ $$err25 GF@$$ta string@string\nJUMPIFNEQ $$err25 GF@$$tb string@float\n",
    "ISINT GF@$$t GF@$$b\nJUMPIFNEQ $$err26 GF@$$t bool@true\n",
    "FLOAT2INT GF@$$i GF@$$b\nSTRLEN GF@$$n GF@$$a\n",
    "LT GF@$$t GF@$$i int@0\nJUMPIFEQ $$ord$zero GF@$$t bool@true\n",
    "LT GF@$$t GF@$$i GF@$$n\nJUMPIFNEQ $$ord$zero GF@$$t bool@true\n",
    "STRI2INT GF@$$n GF@$$a GF@$$i\nINT2FLOAT GF@$$r GF@$$n\nPUSHS GF@$$r\nRETURN\n",
    "LABEL $$ord$zero\nPUSHS float@0x0p+0\nRETURN\n",
    "LABEL $$b_chr\nPOPS GF@$$a\nTYPE GF@$$ta GF@$$a\nJUMPIFNEQ $$err25 GF@$$ta string@float\n",
    "ISINT GF@$$t GF@$$a\nJUMPIFNEQ $$err26 GF@$$t bool@true\n",
    "FLOAT2INT GF@$$n GF@$$a\nINT2CHAR GF@$$r GF@$$n\nPUSHS GF@$$r\nRETURN\n",
    // substring(s, i, j): j -> GF@$$j, i -> GF@$$i, s -> GF@$$a
    "LABEL $$b_substring\nPOPS GF@$$j\nPOPS GF@$$i\nPOPS GF@$$a\n",
    "TYPE GF@$$ta GF@$$a\nJUMPIFNEQ $$err25 GF@$$ta string@string\n",
    "TYPE GF@$$ta GF@$$i\nJUMPIFNEQ $$err25 GF@$$ta string@float\n",
    "TYPE GF@$$ta GF@$$j\nJUMPIFNEQ $$err25 GF@$$ta string@float\n",
    "ISINT GF@$$t GF@$$i\nJUMPIFNEQ $$err26 GF@$$t bool@true\n",
    "ISINT GF@$$t GF@$$j\nJUMPIFNEQ $$err26 GF@$$t bool@true\n",
    "FLOAT2INT GF@$$i GF@$$i\nFLOAT2INT GF@$$j GF@$$j\nSTRLEN GF@$$n GF@$$a\n",
    "LT GF@$$t GF@$$i int@0\nJUMPIFEQ $$substr$null GF@$$t bool@true\n",
    "LT GF@$$t GF@$$j int@0\nJUMPIFEQ $$substr$null GF@$$t bool@true\n",
    "GT GF@$$t GF@$$i GF@$$j\nJUMPIFEQ $$substr$null GF@$$t bool@true\n",
    "LT GF@$$t GF@$$i GF@$$n\nJUMPIFNEQ $$substr$null GF@$$t bool@true\n",
    "GT GF@$$t GF@$$j GF@$$n\nJUMPIFEQ $$substr$null GF@$$t bool@true\n",
    "MOVE GF@$$r string@\n",
    "LABEL $$substr$loop\nJUMPIFEQ $$substr$done GF@$$i GF@$$j\n",
    "GETCHAR GF@$$t GF@$$a GF@$$i\nCONCAT GF@$$r GF@$$r GF@$$t\nADD GF@$$i GF@$$i int@1\nJUMP $$substr$loop\n",
    "LABEL $$substr$done\nPUSHS GF@$$r\nRETURN\n",
    "LABEL $$substr$null\nPUSHS nil@nil\nRETURN\n",
};

/* ---------- program ---------- */

static void define_global(const char *name, void *value, void *ctx) {
    (void) value;
    Gen *g = ctx;
    EMIT("DEFVAR GF@%s", name);
    EMIT("MOVE GF@%s nil@nil", name);
}

void generate_program(const Program *program, FILE *out) {
    Gen gen = {.out = out};
    Gen *g = &gen;

    EMIT(".IFJcode25");
    for (size_t i = 0; i < sizeof TEMPS / sizeof TEMPS[0]; i++) EMIT("DEFVAR GF@$$%s", TEMPS[i]);
    symtable_foreach(&program->globals, define_global, g);
    EMIT("CREATEFRAME");
    EMIT("CALL $f$main$0");
    EMIT("POPS GF@$$discard");
    EMIT("EXIT int@0");
    for (size_t i = 0; i < sizeof RUNTIME / sizeof RUNTIME[0]; i++) fputs(RUNTIME[i], out);
    for (size_t i = 0; i < program->funcs.count; i++) gen_function(g, program->funcs.items[i]);
}
