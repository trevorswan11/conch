#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "util/mem.h"

static const size_t MAX_OPERATOR_LEN = 3;

typedef enum {
    END = 0,

    // Identifiers and literals
    IDENT,
    INT,
    FLOAT,

    // Operators
    ASSIGN,
    PLUS,
    PLUS_ASSIGN,
    MINUS,
    MINUS_ASSIGN,
    ASTERISK,
    ASTERISK_ASSIGN,
    SLASH,
    SLASH_ASSIGN,
    BANG,

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

    LT,
    LTEQ,
    GT,
    GTEQ,
    EQ,
    NEQ,

    DOT,
    DOT_DOT,
    DOT_DOT_EQ,

    // DELIMITERS
    COMMA,
    SEMICOLON,

    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,
    LBRACKET,
    RBRACKET,

    // Keywords
    FUNCTION,
    LET,
    STRUCT,
    TRUE,
    FALSE,
    BOOLEAN_AND,
    BOOLEAN_OR,

    ILLEGAL,
} TokenType;

// Converts the provided char into its token type representation.
//
// If the character is not a valid delimiter, t is not modified.
static inline bool misc_token_type_from_char(char c, TokenType* t) {
    switch (c) {
    case ',':
        *t = COMMA;
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
