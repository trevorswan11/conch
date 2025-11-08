#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "util/mem.h"

typedef enum {
    END = 0,

    // Identifiers and literals
    IDENT,
    INT_10,
    INT_2,
    INT_8,
    INT_16,
    FLOAT,

    // Operators
    ASSIGN,
    WALRUS,
    PLUS,
    PLUS_ASSIGN,
    PLUS_PLUS,
    MINUS,
    MINUS_ASSIGN,
    MINUS_MINUS,
    STAR,
    STAR_ASSIGN,
    STAR_STAR,
    SLASH,
    SLASH_ASSIGN,
    PERCENT,
    PERCENT_ASSIGN,
    BANG,
    WHAT,

    AND,
    AND_ASSIGN,
    OR,
    OR_ASSIGN,
    SHL,
    SHL_ASSIGN,
    SHR,
    SHR_ASSIGN,
    NOT,
    NOT_ASSIGN,
    XOR,
    XOR_ASSIGN,

    LT,
    LTEQ,
    GT,
    GTEQ,
    EQ,
    NEQ,

    DOT,
    DOT_DOT,
    DOT_DOT_EQ,
    ARROW,
    FAT_ARROW,
    COMMENT,
    MULTILINE_STRING,

    // DELIMITERS
    COMMA,
    COLON,
    SEMICOLON,

    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,
    LBRACKET,
    RBRACKET,

    SINGLE_QUOTE,
    DOUBLE_QUOTE,

    // Keywords
    FUNCTION,
    VAR,
    CONST,
    STATIC,
    STRUCT,
    ENUM,
    TRUE,
    FALSE,
    BOOLEAN_AND,
    BOOLEAN_OR,
    IS,
    IF,
    ELSE,
    MATCH,
    CASE,
    RETURN,
    FOR,
    WHILE,
    DO,
    CONTINUE,
    BREAK,
    NIL,
    TYPEOF,
    IMPORT,
    FROM,

    ILLEGAL,
} TokenType;

static const char* const TOKEN_TYPE_NAMES[] = {
    "END",

    // Identifiers and literals
    "IDENT",
    "INT_10",
    "INT_2",
    "INT_8",
    "INT_16",
    "FLOAT",

    // Operators
    "ASSIGN",
    "WALRUS",
    "PLUS",
    "PLUS_ASSIGN",
    "PLUS_PLUS",
    "MINUS",
    "MINUS_ASSIGN",
    "MINUS_MINUS",
    "STAR",
    "STAR_ASSIGN",
    "STAR_STAR",
    "SLASH",
    "SLASH_ASSIGN",
    "PERCENT",
    "PERCENT_ASSIGN",
    "BANG",
    "WHAT",

    "AND",
    "AND_ASSIGN",
    "OR",
    "OR_ASSIGN",
    "SHL",
    "SHL_ASSIGN",
    "SHR",
    "SHR_ASSIGN",
    "NOT",
    "NOT_ASSIGN",
    "XOR",
    "XOR_ASSIGN",

    "LT",
    "LTEQ",
    "GT",
    "GTEQ",
    "EQ",
    "NEQ",

    "DOT",
    "DOT_DOT",
    "DOT_DOT_EQ",
    "ARROW",
    "FAT_ARROW",
    "COMMENT",
    "MULTILINE_STRING",

    // Delimiters
    "COMMA",
    "COLON",
    "SEMICOLON",

    "LPAREN",
    "RPAREN",
    "LBRACE",
    "RBRACE",
    "LBRACKET",
    "RBRACKET",

    "SINGLE_QUOTE",
    "DOUBLE_QUOTE",

    // Keywords
    "FUNCTION",
    "VAR",
    "CONST",
    "STATIC",
    "STRUCT",
    "ENUM",
    "TRUE",
    "FALSE",
    "BOOLEAN_AND",
    "BOOLEAN_OR",
    "IS",
    "IF",
    "ELSE",
    "MATCH",
    "CASE",
    "RETURN",
    "FOR",
    "WHILE",
    "DO",
    "CONTINUE",
    "BREAK",
    "NIL",
    "TYPEOF",
    "IMPORT",
    "FROM",

    "ILLEGAL",
};

static inline const char* token_type_name(TokenType type) {
    return TOKEN_TYPE_NAMES[type];
}

// Converts the provided char into its token type representation.
//
// If the character is not a valid delimiter, t is not modified.
static inline bool misc_token_type_from_char(char c, TokenType* t) {
    switch (c) {
    case ',':
        *t = COMMA;
        return true;
    case ':':
        *t = COLON;
        return true;
    case ';':
        *t = SEMICOLON;
        return true;
    case '(':
        *t = LPAREN;
        return true;
    case ')':
        *t = RPAREN;
        return true;
    case '{':
        *t = LBRACE;
        return true;
    case '}':
        *t = RBRACE;
        return true;
    case '[':
        *t = LBRACKET;
        return true;
    case ']':
        *t = RBRACKET;
        return true;
    case '\'':
        *t = SINGLE_QUOTE;
        return true;
    case '\"':
        *t = DOUBLE_QUOTE;
        return true;
    default:
        return false;
    }
}

// A stack allocated Token that does not own its string literal.
//
// The internal literal is not necessarily null terminated, so use the stored length when possible.
typedef struct {
    TokenType type;
    Slice     slice;
} Token;

static inline Token token_init(TokenType t, const char* l, size_t length) {
    return (Token){.type = t, .slice = slice_from_s(l, length)};
}
