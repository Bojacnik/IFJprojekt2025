#ifndef IFJCODE25_TOKEN_H
#define IFJCODE25_TOKEN_H

typedef enum TokenType {
    TKTYPE_KEYWORD,
    TKTYPE_IDENTIFIER,
    TKTYPE_PUNCTUATION,
    TKTYPE_OPERATOR,
    TKTYPE_VARIABLE,
    TKTYPE_LITERAL_INT,
    TKTYPE_LITERAL_FLOAT,
    TKTYPE_LITERAL_STRING,
    TKTYPE_LITERAL_NIL,
    TKTYPE_LITERAL_BOOL,
    TKTYPE_EOF
} TokenType;

typedef enum KeywordType {
    KWTYPE_NONE,
    KWTYPE_IMPORT,
    KWTYPE_FOR,
    KWTYPE_CLASS,
    KWTYPE_STATIC,
    KWTYPE_IF,
    KWTYPE_ELSE,
    KWTYPE_WHILE,
    KWTYPE_IFJ,
    KWTYPE_IS,
    KWTYPE_NULL,
    KWTYPE_NUM,
    KWTYPE_RETURN,
    KWTYPE_STRING,
    KWTYPE_VAR,
} KeywordType;

typedef enum PunctuationType {
    PTTYPE_OPENPARENTHESIS,
    PTTYPE_CLOSEPARENTHESIS,
    PTTYPE_OPENBRACKET,
    PTTYPE_CLOSEBRACKET,
    PTTYPE_OPENBRACE,
    PTTYPE_CLOSEBRACE,
    PTTYPE_UNDERSCORE,
    PTTYPE_COMMA
} PunctuationType;

typedef enum OperatorType {
    // Arithmetic operators
    OPTYPE_PLUS,
    OPTYPE_MINUS,
    OPTYPE_MULTIPLY,
    OPTYPE_DIVIDE,
    OPTYPE_MODULE,

    // Logic operators
    OPTYPE_AND,
    OPTYPE_OR,
    OPTYPE_XOR,
    OPTYPE_NOT,

    // Logic operators
    OPTYPE_EQUAL,
    OPTYPE_NOTEQUAL,
    OPTYPE_LESS,
    OPTYPE_GREATER,
    OPTYPE_GREATEREQUAL,
    OPTYPE_LESSEQUAL,
} OperatorType;

typedef struct Token {
    TokenType type;

    union {
        KeywordType keyword_type;
        const char *identifier;
        PunctuationType punctuation_type;
        OperatorType operator_type;
        int int_value;
        float float_value;
        const char *string_value;
        bool bool_value;
    };
} Token;

#endif
