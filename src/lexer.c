/*
 * Implementace překladače imperativního jazyka IFJ25
 * Lexical analyser (finite automaton, see docs/lexer-fsm.md).
 * Autoři: TODO (login, jméno)
 *
 * Every call of lexer_next() runs the automaton from S_START until it accepts
 * one token. The automaton only ever looks at the current character (peek);
 * a character is consumed (advance) once it is known to belong to the token,
 * so no character ever needs to be pushed back.
 */

#include "lexer.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"

typedef enum {
    S_START,
    // identifiers
    S_IDENT, S_UNDERSCORE, S_GID,
    // numbers
    S_ZERO, S_INT, S_HEX_PREFIX, S_HEX, S_FRAC_FIRST, S_FRAC, S_EXP_FIRST, S_EXP_SIGN, S_EXP,
    // strings
    S_QUOTE1, S_QUOTE2, S_STR, S_STR_ESC, S_STR_HEX1, S_STR_HEX2, S_ML_STR, S_ML_Q1, S_ML_Q2,
    // comments
    S_SLASH, S_LINE_COMMENT, S_BLOCK, S_BLOCK_STAR, S_BLOCK_SLASH,
    // two-character operators
    S_LT, S_GT, S_ASSIGN, S_BANG,
} State;

static const struct {
    const char *text;
    TokenKind kind;
} KEYWORDS[] = {
    {"class", TK_KW_CLASS}, {"else", TK_KW_ELSE}, {"for", TK_KW_FOR},
    {"if", TK_KW_IF}, {"import", TK_KW_IMPORT}, {"is", TK_KW_IS},
    {"null", TK_KW_NULL}, {"return", TK_KW_RETURN}, {"static", TK_KW_STATIC},
    {"var", TK_KW_VAR}, {"while", TK_KW_WHILE}, {"Ifj", TK_KW_IFJ},
    {"Num", TK_KW_TYPE_NUM}, {"String", TK_KW_TYPE_STRING}, {"Null", TK_KW_TYPE_NULL},
};

/* ---------- character input ---------- */

static int peek(const Lexer *l) { return l->cur; }

static void advance(Lexer *l) {
    if (l->cur == EOF) return;
    if (l->cur == '\n') {
        l->line++;
        l->col = 1;
    } else {
        l->col++;
    }
    l->cur = getc(l->in);
}

// Consume the current character and append it to the lexeme.
static void take(Lexer *l) {
    StringBuilder_Add(l->buf, (char) l->cur);
    advance(l);
}

static bool is_ident_char(int c) { return c != EOF && (isalnum(c) || c == '_'); }

static bool is_digit(int c) { return c >= '0' && c <= '9'; }

static bool is_hex(int c) { return c != EOF && isxdigit(c); }

static int hex_value(int c) { return is_digit(c) ? c - '0' : tolower(c) - 'a' + 10; }

static const char *describe(int c) {
    static char text[16];
    if (c == EOF) return "end of file";
    if (c == '\n') return "newline";
    if (c >= 32 && c < 127) snprintf(text, sizeof text, "'%c'", c);
    else snprintf(text, sizeof text, "byte 0x%02x", c);
    return text;
}

/* ---------- the EOL rule ---------- */

// A newline is not emitted as EOL at the start of the file, after another EOL,
// or after a token behind which the spec allows a line break:
// opening parenthesis, dot, comma, operators and '='.
static bool eol_suppressed(const Lexer *l) {
    if (!l->has_last) return true;
    switch (l->last) {
        case TK_EOL:
        case TK_LPAREN: case TK_DOT: case TK_COMMA:
        case TK_MUL: case TK_DIV: case TK_PLUS: case TK_MINUS:
        case TK_LT: case TK_GT: case TK_LE: case TK_GE: case TK_EQ: case TK_NE:
        case TK_KW_IS: case TK_ASSIGN:
            return true;
        default:
            return false;
    }
}

/* ---------- token construction ---------- */

static Token emit(Lexer *l, TokenKind kind, int line, int col) {
    l->last = kind;
    l->has_last = true;
    return (Token){.kind = kind, .line = line, .col = col};
}

static Token emit_string(Lexer *l, int line, int col) {
    Token t = emit(l, TK_STRING, line, col);
    t.len = l->buf->count;
    t.str = StringBuilder_ToString(l->buf);
    return t;
}

// Multiline literal: whitespace-only text between the opening """ and the first
// newline is dropped (with that newline); whitespace-only text between the last
// newline and the closing """ is dropped (with that newline).
static void trim_multiline(StringBuilder *sb) {
    char *s = sb->buffer;
    size_t n = sb->count;

    size_t i = 0;
    while (i < n && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r')) i++;
    if (i < n && s[i] == '\n') {
        memmove(s, s + i + 1, n - i - 1);
        n -= i + 1;
    }

    size_t j = n;
    while (j > 0 && (s[j - 1] == ' ' || s[j - 1] == '\t' || s[j - 1] == '\r')) j--;
    if (j > 0 && s[j - 1] == '\n') n = j - 1;

    sb->count = n;
    s[n] = '\0';
}

static Token emit_ident_or_keyword(Lexer *l, int line, int col) {
    const char *text = StringBuilder_CStr(l->buf);
    for (size_t k = 0; k < sizeof KEYWORDS / sizeof KEYWORDS[0]; k++) {
        if (strcmp(text, KEYWORDS[k].text) == 0) return emit(l, KEYWORDS[k].kind, line, col);
    }
    Token t = emit(l, TK_IDENT, line, col);
    t.str = StringBuilder_ToString(l->buf);
    return t;
}

static Token emit_number(Lexer *l, TokenKind kind, bool hex, int line, int col) {
    Token t = emit(l, kind, line, col);
    const char *text = StringBuilder_CStr(l->buf);
    if (kind == TK_FLOAT) t.f = strtod(text, NULL);
    else t.i = hex ? strtoll(text + 2, NULL, 16) : strtoll(text, NULL, 10);
    return t;
}

/* ---------- public interface ---------- */

void lexer_init(Lexer *l, FILE *in) {
    l->in = in;
    l->line = 1;
    l->col = 1;
    l->cur = getc(in);
    l->last = TK_EOF;
    l->has_last = false;
    l->buf = StringBuilder_ctor(64);
}

void lexer_free(Lexer *l) {
    StringBuilder_dtor(l->buf);
    l->buf = NULL;
}

#define LEX_ERROR(...) fatal(ERR_LEX, line, col, __VA_ARGS__)

Token lexer_next(Lexer *l) {
    State state = S_START;
    int line = l->line, col = l->col; // start of the current lexeme
    int depth = 0;                    // block comment nesting
    int hex_hi = 0;                   // first digit of \xdd

    StringBuilder_Clear(l->buf);

    for (;;) {
        const int c = peek(l);

        switch (state) {
            case S_START:
                line = l->line;
                col = l->col;
                if (c == EOF) return emit(l, TK_EOF, line, col);
                if (c == ' ' || c == '\t' || c == '\r') { advance(l); break; }
                if (c == '\n') {
                    advance(l);
                    if (eol_suppressed(l)) break;
                    return emit(l, TK_EOL, line, col);
                }
                if (isalpha(c)) { take(l); state = S_IDENT; break; }
                if (c == '_') { take(l); state = S_UNDERSCORE; break; }
                if (c == '0') { take(l); state = S_ZERO; break; }
                if (is_digit(c)) { take(l); state = S_INT; break; }

                advance(l);
                switch (c) {
                    case '"': state = S_QUOTE1; break;
                    case '/': state = S_SLASH; break;
                    case '<': state = S_LT; break;
                    case '>': state = S_GT; break;
                    case '=': state = S_ASSIGN; break;
                    case '!': state = S_BANG; break;
                    case '(': return emit(l, TK_LPAREN, line, col);
                    case ')': return emit(l, TK_RPAREN, line, col);
                    case '{': return emit(l, TK_LBRACE, line, col);
                    case '}': return emit(l, TK_RBRACE, line, col);
                    case ',': return emit(l, TK_COMMA, line, col);
                    case '.': return emit(l, TK_DOT, line, col);
                    case '*': return emit(l, TK_MUL, line, col);
                    case '+': return emit(l, TK_PLUS, line, col);
                    case '-': return emit(l, TK_MINUS, line, col);
                    default: LEX_ERROR("unexpected %s", describe(c));
                }
                break;

            /* ----- identifiers: letter (letter|digit|_)*  and  __ (letter|digit|_)* ----- */

            case S_IDENT:
                if (is_ident_char(c)) { take(l); break; }
                return emit_ident_or_keyword(l, line, col);

            case S_UNDERSCORE:
                if (c != '_') LEX_ERROR("identifier must start with a letter (or '__' for a global)");
                take(l);
                state = S_GID;
                break;

            case S_GID:
                if (is_ident_char(c)) { take(l); break; }
                {
                    Token t = emit(l, TK_GID, line, col);
                    t.str = StringBuilder_ToString(l->buf);
                    return t;
                }

            /* ----- numbers ----- */

            case S_ZERO:
                if (c == 'x') { take(l); state = S_HEX_PREFIX; break; }
                if (is_digit(c)) LEX_ERROR("leading zeros are not allowed");
                if (c == '.') { take(l); state = S_FRAC_FIRST; break; }
                if (c == 'e' || c == 'E') { take(l); state = S_EXP_FIRST; break; }
                if (is_ident_char(c)) LEX_ERROR("invalid number");
                return emit_number(l, TK_INT, false, line, col);

            case S_INT:
                if (is_digit(c)) { take(l); break; }
                if (c == '.') { take(l); state = S_FRAC_FIRST; break; }
                if (c == 'e' || c == 'E') { take(l); state = S_EXP_FIRST; break; }
                if (is_ident_char(c)) LEX_ERROR("invalid number");
                return emit_number(l, TK_INT, false, line, col);

            case S_HEX_PREFIX:
                if (!is_hex(c)) LEX_ERROR("expected a hexadecimal digit after '0x'");
                take(l);
                state = S_HEX;
                break;

            case S_HEX:
                if (is_hex(c)) { take(l); break; }
                if (is_ident_char(c)) LEX_ERROR("invalid hexadecimal number");
                return emit_number(l, TK_INT, true, line, col);

            case S_FRAC_FIRST:
                if (!is_digit(c)) LEX_ERROR("expected a digit after '.'");
                take(l);
                state = S_FRAC;
                break;

            case S_FRAC:
                if (is_digit(c)) { take(l); break; }
                if (c == 'e' || c == 'E') { take(l); state = S_EXP_FIRST; break; }
                if (is_ident_char(c)) LEX_ERROR("invalid number");
                return emit_number(l, TK_FLOAT, false, line, col);

            case S_EXP_FIRST:
                if (c == '+' || c == '-') { take(l); state = S_EXP_SIGN; break; }
                if (!is_digit(c)) LEX_ERROR("expected a digit in the exponent");
                take(l);
                state = S_EXP;
                break;

            case S_EXP_SIGN:
                if (!is_digit(c)) LEX_ERROR("expected a digit in the exponent");
                take(l);
                state = S_EXP;
                break;

            case S_EXP:
                if (is_digit(c)) { take(l); break; }
                if (is_ident_char(c)) LEX_ERROR("invalid number");
                return emit_number(l, TK_FLOAT, false, line, col);

            /* ----- strings ----- */

            case S_QUOTE1: // after "
                if (c == '"') { advance(l); state = S_QUOTE2; break; }
                state = S_STR; // re-examine c as string content
                break;

            case S_QUOTE2: // after "" : either the empty string or the start of """
                if (c == '"') { advance(l); state = S_ML_STR; break; }
                return emit_string(l, line, col);

            case S_STR:
                if (c == '"') { advance(l); return emit_string(l, line, col); }
                if (c == '\\') { advance(l); state = S_STR_ESC; break; }
                if (c == EOF || c == '\n') LEX_ERROR("unterminated string literal");
                if (c < 32) LEX_ERROR("control character %s in string literal", describe(c));
                take(l);
                break;

            case S_STR_ESC: {
                char out;
                switch (c) {
                    case '"': out = '"'; break;
                    case 'n': out = '\n'; break;
                    case 'r': out = '\r'; break;
                    case 't': out = '\t'; break;
                    case '\\': out = '\\'; break;
                    case 'x': advance(l); state = S_STR_HEX1; continue;
                    default: LEX_ERROR("invalid escape sequence '\\' followed by %s", describe(c));
                }
                StringBuilder_Add(l->buf, out);
                advance(l);
                state = S_STR;
                break;
            }

            case S_STR_HEX1:
                if (!is_hex(c)) LEX_ERROR("'\\x' must be followed by two hexadecimal digits");
                hex_hi = hex_value(c);
                advance(l);
                state = S_STR_HEX2;
                break;

            case S_STR_HEX2:
                if (!is_hex(c)) LEX_ERROR("'\\x' must be followed by two hexadecimal digits");
                StringBuilder_Add(l->buf, (char) (hex_hi * 16 + hex_value(c)));
                advance(l);
                state = S_STR;
                break;

            case S_ML_STR: // inside """ ... """, no escape sequences
                if (c == EOF) LEX_ERROR("unterminated multiline string literal");
                if (c == '"') { advance(l); state = S_ML_Q1; break; }
                take(l);
                break;

            case S_ML_Q1: // one " seen inside a multiline literal
                if (c == '"') { advance(l); state = S_ML_Q2; break; }
                StringBuilder_Add(l->buf, '"');
                state = S_ML_STR;
                break;

            case S_ML_Q2: // two " seen inside a multiline literal
                if (c == '"') {
                    advance(l);
                    trim_multiline(l->buf);
                    return emit_string(l, line, col);
                }
                StringBuilder_AddStr(l->buf, "\"\"");
                state = S_ML_STR;
                break;

            /* ----- '/' : division or comments ----- */

            case S_SLASH:
                if (c == '/') { advance(l); state = S_LINE_COMMENT; break; }
                if (c == '*') { advance(l); depth = 1; state = S_BLOCK; break; }
                return emit(l, TK_DIV, line, col);

            case S_LINE_COMMENT:
                // The newline is left for S_START: a line comment counts as a newline.
                if (c == '\n' || c == EOF) { state = S_START; break; }
                advance(l);
                break;

            case S_BLOCK: // block comments nest and count as a single space
                if (c == EOF) LEX_ERROR("unterminated block comment");
                advance(l);
                if (c == '*') state = S_BLOCK_STAR;
                else if (c == '/') state = S_BLOCK_SLASH;
                break;

            case S_BLOCK_STAR:
                if (c == '/') {
                    advance(l);
                    state = --depth == 0 ? S_START : S_BLOCK;
                } else if (c == '*') {
                    advance(l);
                } else {
                    state = S_BLOCK;
                }
                break;

            case S_BLOCK_SLASH:
                if (c == '*') {
                    advance(l);
                    depth++;
                    state = S_BLOCK;
                } else if (c == '/') {
                    advance(l);
                } else {
                    state = S_BLOCK;
                }
                break;

            /* ----- two-character operators ----- */

            case S_LT:
                if (c == '=') { advance(l); return emit(l, TK_LE, line, col); }
                return emit(l, TK_LT, line, col);

            case S_GT:
                if (c == '=') { advance(l); return emit(l, TK_GE, line, col); }
                return emit(l, TK_GT, line, col);

            case S_ASSIGN:
                if (c == '=') { advance(l); return emit(l, TK_EQ, line, col); }
                return emit(l, TK_ASSIGN, line, col);

            case S_BANG:
                if (c == '=') { advance(l); return emit(l, TK_NE, line, col); }
                LEX_ERROR("'!' must be followed by '='");
        }
    }
}
