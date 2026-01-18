#pragma once

#include <expected>
#include <optional>
#include <string_view>

#include "core.hpp"

enum class TokenError {
    NON_STRING_TOKEN,
    UNEXPECTED_CHAR,
};

enum class TokenType {
    END,

    IDENT,
    INT_2,
    INT_8,
    INT_10,
    INT_16,
    UINT_2,
    UINT_8,
    UINT_10,
    UINT_16,
    UZINT_2,
    UZINT_8,
    UZINT_10,
    UZINT_16,
    FLOAT,
    STRING,
    CHARACTER,

    ASSIGN,
    WALRUS,
    PLUS,
    MINUS,
    STAR,
    STAR_STAR,
    SLASH,
    PERCENT,
    BANG,
    WHAT,

    AND,
    OR,
    SHL,
    SHR,
    NOT,
    XOR,

    PLUS_ASSIGN,
    MINUS_ASSIGN,
    STAR_ASSIGN,
    SLASH_ASSIGN,
    PERCENT_ASSIGN,
    AND_ASSIGN,
    OR_ASSIGN,
    SHL_ASSIGN,
    SHR_ASSIGN,
    NOT_ASSIGN,
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
    FAT_ARROW,

    COMMENT,
    MULTILINE_STRING,

    COMMA,
    COLON,
    SEMICOLON,
    COLON_COLON,

    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,
    LBRACKET,
    RBRACKET,

    SINGLE_QUOTE,
    UNDERSCORE,
    REF,
    WITH,

    FUNCTION,
    VAR,
    CONST,
    STRUCT,
    ENUM,
    TRUE,
    FALSE,
    BOOLEAN_AND,
    BOOLEAN_OR,
    IS,
    IN,
    IF,
    ELSE,
    MATCH,
    RETURN,
    LOOP,
    FOR,
    WHILE,
    CONTINUE,
    BREAK,
    NIL,
    TYPEOF,
    IMPORT,
    TYPE,
    IMPL,
    ORELSE,
    DO,
    AS,

    INT_TYPE,
    UINT_TYPE,
    SIZE_TYPE,
    BYTE_TYPE,
    FLOAT_TYPE,
    STRING_TYPE,
    BOOL_TYPE,
    VOID_TYPE,

    ILLEGAL,
};

enum class Base {
    UNKNOWN     = 0,
    BINARY      = 2,
    OCTAL       = 8,
    DECIMAL     = 10,
    HEXADECIMAL = 16,
};

auto base_idx(Base base) noexcept -> int;
auto digit_in_base(byte c, Base base) noexcept -> bool;

namespace token_type {
auto intoIntBase(TokenType type) -> Base;
auto miscFromChar(byte c) -> std::optional<TokenType>;
auto isSignedInt(TokenType t) -> bool;
auto isUnsignedInt(TokenType t) -> bool;
auto isSizeInt(TokenType t) -> bool;
auto isInt(TokenType t) -> bool;
} // namespace token_type

struct Token {
    TokenType        type;
    std::string_view slice;
    size_t           line;
    size_t           column;

    [[nodiscard]] auto promote() const -> std::expected<std::string, TokenError>;
};
