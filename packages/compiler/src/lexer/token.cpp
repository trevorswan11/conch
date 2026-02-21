#include <algorithm>
#include <cassert>
#include <cctype>

#include "lexer/keywords.hpp"
#include "lexer/token.hpp"

namespace conch {

auto base_idx(Base base) noexcept -> int {
    switch (base) {
    case Base::BINARY:      return 0;
    case Base::OCTAL:       return 1;
    case Base::DECIMAL:     return 2;
    case Base::HEXADECIMAL: return 3;
    }
}

auto digit_in_base(byte c, Base base) noexcept -> bool {
    switch (base) {
    case Base::BINARY:      return c == '0' || c == '1';
    case Base::OCTAL:       return c >= '0' && c <= '7';
    case Base::DECIMAL:     return std::isdigit(c);
    case Base::HEXADECIMAL: return std::isxdigit(c);
    default:                std::unreachable();
    }
}

namespace token_type {

auto to_base(TokenType tt) noexcept -> Optional<Base> {
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
    default:                  return nullopt;
    }
}

auto misc_from_char(byte c) noexcept -> Optional<TokenType> {
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
    default:  return nullopt;
    }
}

using SuffixMapping                = std::pair<bool (*)(TokenType), usize>;
constexpr auto INT_SUFFIX_MAPPINGS = std::to_array<SuffixMapping>({
    {is_signed_int, 0},
    {is_signed_long_int, 1},
    {is_isize_int, 1},
    {is_unsigned_int, 1},
    {is_unsigned_long_int, 2},
    {is_usize_int, 2},
});

auto suffix_length(TokenType tt) noexcept -> usize {
    if (tt < TokenType::INT_2 || tt > TokenType::UZINT_16) { return 0; }
    return std::ranges::find_if(
               INT_SUFFIX_MAPPINGS,
               [tt](auto in_range) { return in_range(tt); },
               &SuffixMapping::first)
        ->second;
}

} // namespace token_type

auto Token::promote() const -> Expected<std::string, Diagnostic<TokenError>> {
    if (type != TokenType::STRING && type != TokenType::MULTILINE_STRING) {
        return Unexpected{Diagnostic{TokenError::NON_STRING_TOKEN, line, column}};
    }

    // Here we can just trim off the start and finish of the string
    if (type == TokenType::STRING) {
        if (slice.size() < 2) {
            return Unexpected{Diagnostic{TokenError::UNEXPECTED_CHAR, line, column}};
        }
        return std::string{slice.begin() + 1, slice.end() - 1};
    }

    std::string builder{};
    builder.reserve(slice.size());

    auto at_line_start = true;
    for (size_t i = 0; i < slice.size(); i++) {
        const auto c = slice[i];

        // Skip a double backslash at start of line to clean the string
        if (at_line_start) {
            if (c == '\\' && i + 1 < slice.size() && slice[i + 1] == '\\') {
                i += 1;
                continue;
            }
            at_line_start = false;
        }

        builder.push_back(c);
        if (c == '\n') { at_line_start = true; }
    }

    return builder;
}

auto Token::is_primitive() const noexcept -> bool {
    return std::ranges::contains(ALL_PRIMITIVES, type);
}

auto Token::is_builtin() const noexcept -> bool {
    return std::ranges::contains(ALL_BUILTINS, type, &Keyword::second);
}

auto Token::is_valid_ident() const noexcept -> bool {
    return type == TokenType::IDENT || is_primitive() || is_builtin();
}

} // namespace conch
