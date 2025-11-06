#pragma once

#include <stdbool.h>

#include "util/mem.h"

typedef enum {
    END = 0,

    // Identifiers and literals
    IDENT,
    INT,
    FLOAT,

    // Operators
    ASSIGN,
    PLUS,

    // DELIMITERS
    COMMA,
    SEMICOLON,

    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,

    // Keywords
    FUNCTION,
    LET,

    ILLEGAL,
} TokenType;

static const char* token_type_strings[ILLEGAL - END + 1] = {
    "EOF",
    "IDENT",
    "INT",
    "FLOAT",
    "=",
    "+",
    ",",
    ";",
    "(",
    ")",
    "{",
    "}",
    "FUNCTION",
    "LET",
    "ILLEGAL",
};

// Converts the provided token type into its stack-allocated string representation.
static inline const char* string_from_token_type(TokenType t) {
    if (t >= END && t <= ILLEGAL) {
        return token_type_strings[t];
    }
    return token_type_strings[ILLEGAL];
}

// Converts the provided char inot its token type representation.
//
// Tokens that cannot be represented by a single byte are considered ILLEGAL by this function.
// In this scenario, `false` is returned.
bool token_type_from_char(char c, TokenType* t);

// A stack allocated Token that does not own its string literal.
//
// The internal literal is not necessarily null terminated, so use the stored length when possible.
typedef struct {
    TokenType type;
    Slice     slice;
} Token;

static inline Token token_init(TokenType t, const char* l, size_t length) {
    return (Token){.type = t, .slice = slice_from(l, length)};
}
