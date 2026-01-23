#pragma once

#include <format>
#include <optional>
#include <string>
#include <string_view>

#include "core.hpp"

enum class TokenError : u8 {
    NON_STRING_TOKEN,
    UNEXPECTED_CHAR,
};

enum class TokenType : u16 {
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

enum class Base : u8 {
    UNKNOWN     = 0,
    BINARY      = 2,
    OCTAL       = 8,
    DECIMAL     = 10,
    HEXADECIMAL = 16,
};

auto base_idx(Base base) noexcept -> int;
auto digit_in_base(byte c, Base base) noexcept -> bool;

namespace token_type {

auto to_base(TokenType type) noexcept -> Base;
auto misc_from_char(byte c) noexcept -> std::optional<TokenType>;
auto is_signed_int(TokenType t) noexcept -> bool;
auto is_unsigned_int(TokenType t) noexcept -> bool;
auto is_size_int(TokenType t) noexcept -> bool;
auto is_int(TokenType t) noexcept -> bool;

} // namespace token_type

struct Token {
    TokenType        type;
    std::string_view slice;
    usize            line;
    usize            column;

    [[nodiscard]] auto promote() const -> Expected<std::string, Diagnostic<TokenError>>;
    auto               primitive() const noexcept -> bool;

    auto operator==(const Token& other) const noexcept -> bool {
        return type == other.type && slice == other.slice && line == other.line &&
               column == other.column;
    }
};

template <> struct std::formatter<Token> : std::formatter<std::string> {
    static constexpr auto parse(std::format_parse_context& ctx) noexcept { return ctx.begin(); }

    template <typename F> auto format(const Token& t, F& ctx) const {
        return std::formatter<std::string>::format(
            std::format("{}({}) [{}, {}]", enum_name(t.type), t.slice, t.line, t.column), ctx);
    }
};
