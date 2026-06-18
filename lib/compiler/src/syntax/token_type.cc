#include "syntax/token_type.hh"

#include <algorithm>
#include <array>
#include <cctype>
#include <utility>

#include <stdx/assert.hh>
#include <stdx/option.hh>
#include <stdx/types.hh>

#include "syntax/builtins.hh"
#include "syntax/keywords.hh"

namespace ghoti::syntax {

auto base_idx(Base base) noexcept -> i32 {
    switch (base) {
    case Base::BINARY:      return 0;
    case Base::OCTAL:       return 1;
    case Base::DECIMAL:     return 2;
    case Base::HEXADECIMAL: return 3;
    }
}

auto digit_in_base(char c, Base base) noexcept -> bool {
    switch (base) {
    case Base::BINARY:      return c == '0' || c == '1';
    case Base::OCTAL:       return c >= '0' && c <= '7';
    case Base::DECIMAL:     return std::isdigit(c);
    case Base::HEXADECIMAL: return std::isxdigit(c);
    default:                UNREACHABLE("Unknown base");
    }
}

namespace token_type {

auto to_base(TokenType tt) noexcept -> stdx::Option<Base> {
    switch (tt) {
    case TokenType::INT_2:
    case TokenType::LINT_2:
    case TokenType::ZINT_2:
    case TokenType::UINT_2:
    case TokenType::ULINT_2:
    case TokenType::UZINT_2:  return Base::BINARY;
    case TokenType::INT_8:
    case TokenType::LINT_8:
    case TokenType::ZINT_8:
    case TokenType::UINT_8:
    case TokenType::ULINT_8:
    case TokenType::UZINT_8:  return Base::OCTAL;
    case TokenType::INT_10:
    case TokenType::LINT_10:
    case TokenType::ZINT_10:
    case TokenType::UINT_10:
    case TokenType::ULINT_10:
    case TokenType::UZINT_10: return Base::DECIMAL;
    case TokenType::INT_16:
    case TokenType::LINT_16:
    case TokenType::ZINT_16:
    case TokenType::UINT_16:
    case TokenType::ULINT_16:
    case TokenType::UZINT_16: return Base::HEXADECIMAL;
    default:                  return stdx::none;
    }
}

auto misc_from_char(char c) noexcept -> stdx::Option<TokenType> {
    switch (c) {
    case ',': return TokenType::COMMA;
    case ':': return TokenType::COLON;
    case ';': return TokenType::SEMICOLON;
    case '(': return TokenType::LPAREN;
    case ')': return TokenType::RPAREN;
    case '{': return TokenType::LBRACE;
    case '}': return TokenType::RBRACE;
    case '[': return TokenType::LBRACKET;
    case ']': return TokenType::RBRACKET;
    case '_': return TokenType::UNDERSCORE;
    default:  return stdx::none;
    }
}

auto is_primitive(TokenType type) noexcept -> bool {
    return std::ranges::contains(ALL_PRIMITIVES, type);
}

auto is_valid_ident(TokenType type) noexcept -> bool {
    switch (type) {
    case TokenType::IDENT:
    case TokenType::NORETURN:
    case TokenType::TYPE_TYPE:
    case TokenType::AUTO_TYPE:
    case TokenType::OPAQUE_TYPE: return true;
    default:                     return is_primitive(type) || get_builtin_opt(type);
    }
}

namespace {

using SuffixMapping = std::pair<bool (*)(TokenType), usize>;
constexpr std::array INT_SUFFIX_MAPPINGS{
    SuffixMapping{is_i32, 0},
    SuffixMapping{is_i64, 1},
    SuffixMapping{is_isize_int, 1},
    SuffixMapping{is_u32, 1},
    SuffixMapping{is_u64, 2},
    SuffixMapping{is_usize_int, 2},
};

} // namespace

auto suffix_length(TokenType tt) noexcept -> usize {
    if (tt == TokenType::F32) { return 1; }
    if (tt < TokenType::INT_2 || tt > TokenType::UZINT_16) { return 0; }

    // Returns the suffix (second of pair) for the first range that returns true
    return std::ranges::find_if(
               INT_SUFFIX_MAPPINGS,
               [tt](auto* in_range) -> bool { return in_range(tt); },
               &SuffixMapping::first)
        ->second;
}

} // namespace token_type

} // namespace ghoti::syntax
