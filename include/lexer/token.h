#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "util/mem.h"
#include "util/status.h"

#define FOREACH_TOKEN(PROCESS)                                                                     \
    PROCESS(END),                                                                                  \
                                                                                                   \
        PROCESS(IDENT), PROCESS(INT_2), PROCESS(INT_8), PROCESS(INT_10), PROCESS(INT_16),          \
        PROCESS(UINT_2), PROCESS(UINT_8), PROCESS(UINT_10), PROCESS(UINT_16), PROCESS(FLOAT),      \
        PROCESS(STRING), PROCESS(CHARACTER),                                                       \
                                                                                                   \
        PROCESS(ASSIGN), PROCESS(WALRUS), PROCESS(PLUS), PROCESS(PLUS_ASSIGN), PROCESS(PLUS_PLUS), \
        PROCESS(MINUS), PROCESS(MINUS_ASSIGN), PROCESS(MINUS_MINUS), PROCESS(STAR),                \
        PROCESS(STAR_ASSIGN), PROCESS(STAR_STAR), PROCESS(SLASH), PROCESS(SLASH_ASSIGN),           \
        PROCESS(PERCENT), PROCESS(PERCENT_ASSIGN), PROCESS(BANG), PROCESS(WHAT),                   \
                                                                                                   \
        PROCESS(AND), PROCESS(AND_ASSIGN), PROCESS(OR), PROCESS(OR_ASSIGN), PROCESS(SHL),          \
        PROCESS(SHL_ASSIGN), PROCESS(SHR), PROCESS(SHR_ASSIGN), PROCESS(NOT), PROCESS(NOT_ASSIGN), \
        PROCESS(XOR), PROCESS(XOR_ASSIGN),                                                         \
                                                                                                   \
        PROCESS(LT), PROCESS(LTEQ), PROCESS(GT), PROCESS(GTEQ), PROCESS(EQ), PROCESS(NEQ),         \
                                                                                                   \
        PROCESS(DOT), PROCESS(DOT_DOT), PROCESS(DOT_DOT_EQ), PROCESS(ARROW), PROCESS(FAT_ARROW),   \
                                                                                                   \
        PROCESS(COMMENT), PROCESS(MULTILINE_STRING),                                               \
                                                                                                   \
        PROCESS(COMMA), PROCESS(COLON), PROCESS(SEMICOLON),                                        \
                                                                                                   \
        PROCESS(LPAREN), PROCESS(RPAREN), PROCESS(LBRACE), PROCESS(RBRACE), PROCESS(LBRACKET),     \
        PROCESS(RBRACKET),                                                                         \
                                                                                                   \
        PROCESS(SINGLE_QUOTE), PROCESS(DOUBLE_QUOTE),                                              \
                                                                                                   \
        PROCESS(FUNCTION), PROCESS(VAR), PROCESS(CONST), PROCESS(STATIC), PROCESS(STRUCT),         \
        PROCESS(ENUM), PROCESS(TRUE), PROCESS(FALSE), PROCESS(BOOLEAN_AND), PROCESS(BOOLEAN_OR),   \
        PROCESS(IS), PROCESS(IF), PROCESS(ELSE), PROCESS(MATCH), PROCESS(CASE), PROCESS(RETURN),   \
        PROCESS(FOR), PROCESS(WHILE), PROCESS(DO), PROCESS(CONTINUE), PROCESS(BREAK),              \
        PROCESS(NIL), PROCESS(TYPEOF), PROCESS(IMPORT), PROCESS(FROM), PROCESS(INT_TYPE),          \
        PROCESS(UINT_TYPE), PROCESS(FLOAT_TYPE),                                                   \
                                                                                                   \
        PROCESS(ILLEGAL)

typedef enum TokenType {
    FOREACH_TOKEN(GENERATE_ENUM),
} TokenType;

static const TokenType TOKEN_TYPES[] = {
    FOREACH_TOKEN(GENERATE_ENUM),
};

static const char* const TOKEN_TYPE_NAMES[] = {
    FOREACH_TOKEN(GENERATE_STRING),
};

const char* token_type_name(TokenType type);

// Converts the provided char into its token type representation.
//
// If the character is not a valid delimiter, t is not modified.
bool misc_token_type_from_char(char c, TokenType* t);
bool token_is_integer(TokenType t);
bool token_is_signed_integer(TokenType t);
bool token_is_unsigned_integer(TokenType t);

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
//
// - Standard strings are stripped of their leading and trailing quotes.
// - Multiline strings are stripped of their internal '\\' line prefixes.
//
// The returned memory is owned by the caller and must be freed.
TRY_STATUS promote_token_string(Token token, MutSlice* slice);
