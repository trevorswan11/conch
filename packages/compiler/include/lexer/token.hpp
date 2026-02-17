#pragma once

#include <format>
#include <string>
#include <string_view>
#include <utility>

#include "core/common.hpp"
#include "core/diagnostic.hpp"
#include "core/reflection.hpp"
#include "core/expected.hpp"
#include "core/optional.hpp"

namespace conch {

enum class TokenError : u8 {
    NON_STRING_TOKEN,
    UNEXPECTED_CHAR,
};

enum class TokenType : u8 {
    END,

    IDENT,

    INT_2,
    INT_8,
    INT_10,
    INT_16,
    LINT_2,
    LINT_8,
    LINT_10,
    LINT_16,
    ZINT_2,
    ZINT_8,
    ZINT_10,
    ZINT_16,

    UINT_2,
    UINT_8,
    UINT_10,
    UINT_16,
    ULINT_2,
    ULINT_8,
    ULINT_10,
    ULINT_16,
    UZINT_2,
    UZINT_8,
    UZINT_10,
    UZINT_16,

    FLOAT,
    STRING,
    BYTE,

    ASSIGN,
    WALRUS,
    PLUS,
    MINUS,
    STAR,
    STAR_STAR,
    SLASH,
    PERCENT,
    BANG,

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
    AND_MUT,

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
    IMPORT,
    TYPE_TYPE,
    ORELSE,
    DO,
    AS,

    INT_TYPE,
    LONG_TYPE,
    ISIZE_TYPE,
    UINT_TYPE,
    ULONG_TYPE,
    USIZE_TYPE,
    BYTE_TYPE,
    FLOAT_TYPE,
    STRING_TYPE,
    BOOL_TYPE,
    VOID_TYPE,

    PRIVATE,
    EXTERN,
    EXPORT,
    PACKED,
    VOLATILE,
    STATIC,
    MUT,
    NORETURN,

    TYPEOF,
    SIZEOF,
    ALIGNOF,
    SIN,
    COS,
    TAN,
    SQRT,
    LOG,
    LOG_10,
    LOG_2,
    MIN,
    MAX,
    MOD,
    DIVMOD,
    TRUNC,
    CAST,
    CEIL,
    FLOOR,
    EXP,
    EXP_2,
    CLZ, // Count leading zeroes
    CTZ, // Count trailing zeroes

    ILLEGAL,
};

enum class Base : u8 {
    BINARY      = 2,
    OCTAL       = 8,
    DECIMAL     = 10,
    HEXADECIMAL = 16,
};

auto base_idx(Base base) noexcept -> int;
auto digit_in_base(byte c, Base base) noexcept -> bool;

namespace token_type {

enum class IntegerCategory : u8 {
    SIGNED_BASE,
    SIGNED_WIDE,
    SIGNED_SIZE,
    UNSIGNED_BASE,
    UNSIGNED_WIDE,
    UNSIGNED_SIZE,
};

consteval auto to_int_category(TokenType tt) noexcept -> IntegerCategory {
    switch (tt) {
    case TokenType::INT_2:
    case TokenType::INT_8:
    case TokenType::INT_10:
    case TokenType::INT_16:   return IntegerCategory::SIGNED_BASE;
    case TokenType::LINT_2:
    case TokenType::LINT_8:
    case TokenType::LINT_10:
    case TokenType::LINT_16:  return IntegerCategory::SIGNED_WIDE;
    case TokenType::ZINT_2:
    case TokenType::ZINT_8:
    case TokenType::ZINT_10:
    case TokenType::ZINT_16:  return IntegerCategory::SIGNED_SIZE;
    case TokenType::UINT_2:
    case TokenType::UINT_8:
    case TokenType::UINT_10:
    case TokenType::UINT_16:  return IntegerCategory::UNSIGNED_BASE;
    case TokenType::ULINT_2:
    case TokenType::ULINT_8:
    case TokenType::ULINT_10:
    case TokenType::ULINT_16: return IntegerCategory::UNSIGNED_WIDE;
    case TokenType::UZINT_2:
    case TokenType::UZINT_8:
    case TokenType::UZINT_10:
    case TokenType::UZINT_16: return IntegerCategory::UNSIGNED_SIZE;
    default:                  std::unreachable();
    }
}

auto to_base(TokenType tt) noexcept -> Optional<Base>;
auto misc_from_char(byte b) noexcept -> Optional<TokenType>;

constexpr auto is_signed_int(TokenType tt) noexcept -> bool {
    return TokenType::INT_2 <= tt && tt <= TokenType::INT_16;
}

constexpr auto is_signed_long_int(TokenType tt) noexcept -> bool {
    return TokenType::LINT_2 <= tt && tt <= TokenType::LINT_16;
}

constexpr auto is_isize_int(TokenType tt) noexcept -> bool {
    return TokenType::ZINT_2 <= tt && tt <= TokenType::ZINT_16;
}

constexpr auto is_unsigned_int(TokenType tt) noexcept -> bool {
    return TokenType::UINT_2 <= tt && tt <= TokenType::UINT_16;
}

constexpr auto is_unsigned_long_int(TokenType tt) noexcept -> bool {
    return TokenType::ULINT_2 <= tt && tt <= TokenType::ULINT_16;
}

constexpr auto is_usize_int(TokenType tt) noexcept -> bool {
    return TokenType::UZINT_2 <= tt && tt <= TokenType::UZINT_16;
}

constexpr auto is_int(TokenType tt) noexcept -> bool {
    return TokenType::INT_2 <= tt && tt <= TokenType::UZINT_16;
}

auto suffix_length(TokenType tt) noexcept -> usize;

} // namespace token_type

struct MinimalSourceLocation {
    usize line   = 0;
    usize column = 0;

    auto operator==(const MinimalSourceLocation& other) const noexcept -> bool {
        return line == other.line && column == other.column;
    }

    [[nodiscard]] auto is_at_start() const noexcept -> bool { return line == 0 && column == 0; }
};

struct Token {
    TokenType             type{};
    std::string_view      slice{};
    MinimalSourceLocation location{};

    Token() noexcept = default;
    Token(TokenType tt, std::string_view tok) noexcept : type{tt}, slice{tok} {}
    Token(TokenType tt, std::string_view slice, usize line, usize column) noexcept
        : type{tt}, slice{slice}, location{line, column} {}

    [[nodiscard]] auto get_line() const noexcept -> usize { return location.line; }
    [[nodiscard]] auto get_column() const noexcept -> usize { return location.column; }

    [[nodiscard]] auto promote() const -> Expected<std::string, Diagnostic<TokenError>>;
    auto               is_primitive() const noexcept -> bool;

    auto operator==(const Token& other) const noexcept -> bool {
        return type == other.type && slice == other.slice && location == other.location;
    }
};

} // namespace conch

template <> struct std::formatter<conch::Token> : std::formatter<std::string> {
    static constexpr auto parse(std::format_parse_context& ctx) noexcept { return ctx.begin(); }

    template <typename F> auto format(const conch::Token& t, F& ctx) const {
        return std::formatter<std::string>::format(
            std::format(
                "{}({}) [{}, {}]", enum_name(t.type), t.slice, t.get_line(), t.get_column()),
            ctx);
    }
};
