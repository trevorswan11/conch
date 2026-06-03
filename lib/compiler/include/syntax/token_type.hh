#pragma once

#include <cstddef>
#include <string_view>
#include <type_traits>
#include <utility>

#include <ankerl/unordered_dense.h>

#include "hash.hh"
#include "option.hh"
#include "types.hh"

namespace porpoise::syntax {

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

    F32,
    F64,
    STRING,
    U8,

    ASSIGN,
    WALRUS,
    PLUS,
    MINUS,
    STAR,
    SLASH,
    PERCENT,
    BANG,

    BW_AND,
    BW_OR,
    SHL,
    SHR,
    NOT,
    CARET,

    PLUS_ASSIGN,
    MINUS_ASSIGN,
    STAR_ASSIGN,
    SLASH_ASSIGN,
    PERCENT_ASSIGN,
    BW_AND_ASSIGN,
    BW_OR_ASSIGN,
    SHL_ASSIGN,
    SHR_ASSIGN,
    NOT_ASSIGN,
    XOR_ASSIGN,

    LT,
    LT_EQ,
    GT,
    GT_EQ,
    EQ,
    NEQ,

    DOT,
    DOT_DOT,
    DOT_DOT_EQ,
    FAT_ARROW,

    COMMENT,
    MULTILINE_STRING,
    NULL_TERMINATED,

    COMMA,
    COLON,
    SEMICOLON,
    COLON_COLON,
    ELLIPSIS,

    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,
    LBRACKET,
    RBRACKET,

    SINGLE_QUOTE,
    UNDERSCORE,
    USING,
    AND_MUT,
    STAR_MUT,
    CARET_MUT,

    FUNCTION,
    VAR,
    CONSTANT,
    CONSTEXPR,
    STRUCT,
    ENUM,
    UNION,
    BOOLEAN_TRUE,
    BOOLEAN_FALSE,
    BOOLEAN_AND,
    BOOLEAN_OR,
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
    AUTO_TYPE,
    OPAQUE_TYPE,
    DO,
    AS,
    DEFER,
    TEST,
    UNDEFINED,

    I32_TYPE,
    I64_TYPE,
    ISIZE_TYPE,
    U32_TYPE,
    U64_TYPE,
    USIZE_TYPE,
    U8_TYPE,
    F32_TYPE,
    F64_TYPE,
    BOOL_TYPE,
    VOID_TYPE,

    PUBLIC,
    EXTERN,
    EXPORT,
    VOLATILE,
    MUT_VOLATILE,
    NORETURN,
    NULLPTR,

    BUILTIN_ALIGN_CAST,
    BUILTIN_PTR_CAST,
    BUILTIN_BIT_CAST,
    BUILTIN_CONST_CAST,
    BUILTIN_VOLATILE_CAST,
    BUILTIN_AS,
    BUILTIN_INT_FROM_PTR,
    BUILTIN_PTR_FROM_INT,
    BUILTIN_PTR_FROM_ARRAY,
    BUILTIN_SLICE_FROM_PTR,
    BUILTIN_ALIGN_OF,
    BUILTIN_SIZE_OF,
    BUILTIN_TYPE_OF,
    BUILTIN_TAG_NAME,
    BUILTIN_MEMCPY,
    BUILTIN_MEMSET,
    BUILTIN_MEMMOVE,
    BUILTIN_MUL_ADD,
    BUILTIN_CLZ, // Count leading zeroes
    BUILTIN_CTZ, // Count trailing zeroes
    BUILTIN_POP_COUNT,
    BUILTIN_SQRT,
    BUILTIN_SIN,
    BUILTIN_COS,
    BUILTIN_TAN,
    BUILTIN_EXP,
    BUILTIN_EXP2,
    BUILTIN_LOG,
    BUILTIN_LOG2,
    BUILTIN_LOG10,
    BUILTIN_ABS,
    BUILTIN_FLOOR,
    BUILTIN_CEIL,

    BUILTIN_PANIC,

    ILLEGAL,
};

enum class Base : u8 {
    BINARY      = 2,
    OCTAL       = 8,
    DECIMAL     = 10,
    HEXADECIMAL = 16,
};

[[nodiscard]] auto base_idx(Base base) noexcept -> int;
[[nodiscard]] auto digit_in_base(byte c, Base base) noexcept -> bool;

namespace token_type {

enum class IntegerCategory : u8 {
    SIGNED_BASE,
    SIGNED_WIDE,
    SIGNED_SIZE,
    UNSIGNED_BASE,
    UNSIGNED_WIDE,
    UNSIGNED_SIZE,
};

[[nodiscard]] consteval auto to_int_category(TokenType tt) noexcept -> IntegerCategory {
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

[[nodiscard]] auto to_base(TokenType tt) noexcept -> opt::Option<Base>;
[[nodiscard]] auto misc_from_char(byte b) noexcept -> opt::Option<TokenType>;

[[nodiscard]] constexpr auto is_i32(TokenType tt) noexcept -> bool {
    return TokenType::INT_2 <= tt && tt <= TokenType::INT_16;
}

[[nodiscard]] constexpr auto is_i64(TokenType tt) noexcept -> bool {
    return TokenType::LINT_2 <= tt && tt <= TokenType::LINT_16;
}

[[nodiscard]] constexpr auto is_isize_int(TokenType tt) noexcept -> bool {
    return TokenType::ZINT_2 <= tt && tt <= TokenType::ZINT_16;
}

[[nodiscard]] constexpr auto is_u32(TokenType tt) noexcept -> bool {
    return TokenType::UINT_2 <= tt && tt <= TokenType::UINT_16;
}

[[nodiscard]] constexpr auto is_u64(TokenType tt) noexcept -> bool {
    return TokenType::ULINT_2 <= tt && tt <= TokenType::ULINT_16;
}

[[nodiscard]] constexpr auto is_usize_int(TokenType tt) noexcept -> bool {
    return TokenType::UZINT_2 <= tt && tt <= TokenType::UZINT_16;
}

[[nodiscard]] constexpr auto is_int(TokenType tt) noexcept -> bool {
    return TokenType::INT_2 <= tt && tt <= TokenType::UZINT_16;
}

[[nodiscard]] constexpr auto is_number(TokenType tt) noexcept -> bool {
    return is_int(tt) || tt == TokenType::F32 || tt == TokenType::F64;
}

[[nodiscard]] auto is_primitive(TokenType type) noexcept -> bool;

// Check whether the token is an ident, primitive type, or builtin function.
[[nodiscard]] auto is_valid_ident(TokenType type) noexcept -> bool;

auto suffix_length(TokenType tt) noexcept -> usize;

} // namespace token_type

struct TypedIdentifier {
    std::string_view name;
    TokenType        type;
};

// Helper for ADL std::get in fixed::HashMap
template <std::size_t I>
[[nodiscard]] constexpr auto get(const syntax::TypedIdentifier& typed) noexcept -> auto& {
    if constexpr (I == 0) {
        return typed.name;
    } else if constexpr (I == 1) {
        return typed.type;
    }
}

} // namespace porpoise::syntax

template <> struct ankerl::unordered_dense::hash<porpoise::syntax::TypedIdentifier> {
    using is_avalanching  = void;
    using TypedIdentifier = porpoise::syntax::TypedIdentifier;

    [[nodiscard]] auto operator()(const TypedIdentifier& type) const noexcept {
        porpoise::hash::Hasher hasher{type.type};
        hasher.combine(type.name);
        return hasher.finalize();
    }
};

template <>
struct std::tuple_size<porpoise::syntax::TypedIdentifier> : std::integral_constant<std::size_t, 2> {
};

template <> struct std::tuple_element<0, porpoise::syntax::TypedIdentifier> {
    using type = std::string_view;
};

template <> struct std::tuple_element<1, porpoise::syntax::TypedIdentifier> {
    using type = porpoise::syntax::TokenType;
};
