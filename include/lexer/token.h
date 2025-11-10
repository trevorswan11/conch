#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "util/mem.h"

typedef enum {
    END = 0,

    // Identifiers and literals
    IDENT,
    INT_2,
    INT_8,
    INT_10,
    INT_16,
    FLOAT,

    STRING,
    CHARACTER,

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
    "INT_2",
    "INT_8",
    "INT_10",
    "INT_16",
    "FLOAT",

    "STRING",
    "CHARACTER",

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

const char* token_type_name(TokenType type);

// Converts the provided char into its token type representation.
//
// If the character is not a valid delimiter, t is not modified.
bool misc_token_type_from_char(char c, TokenType* t);
bool token_is_integer(TokenType t);

// A stack allocated Token that does not own its string literal.
//
// The internal literal is not necessarily null terminated, so use the stored length when possible.
typedef struct {
    TokenType type;
    Slice     slice;
    size_t    line;
    size_t    column;
} Token;

Token token_init(TokenType t, const char* str, size_t length, size_t line, size_t col);

// Takes a parsed multiline string-based token and promotes it into a heap allocated string.
// The governing slice is stack allocated but its data is not.
//
// - Standard strings are stripped of their leading and trailing quotes.
// - Multiline strings are stripped of their internal '\\' line prefixes.
//
// The returned memory is owned by the caller and must be freed.
MutSlice promote_token_string(Token token);
