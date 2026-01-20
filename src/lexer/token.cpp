#include <algorithm>
#include <cctype>
#include <expected>
#include <string>
#include <utility>

#include "lexer/keywords.hpp"
#include "lexer/token.hpp"

auto base_idx(Base base) noexcept -> int {
    switch (base) {
    case Base::UNKNOWN: return -1;
    case Base::BINARY: return 0;
    case Base::OCTAL: return 1;
    case Base::DECIMAL: return 2;
    case Base::HEXADECIMAL: return 3;
    }
}

auto digit_in_base(byte c, Base base) noexcept -> bool {
    switch (base) {
    case Base::BINARY: return c == '0' || c == '1';
    case Base::OCTAL: return c >= '0' && c <= '7';
    case Base::DECIMAL: return std::isdigit(c);
    case Base::HEXADECIMAL: return std::isxdigit(c);
    default: std::unreachable();
    }
}

namespace token_type {
auto intoIntBase(TokenType type) noexcept -> Base {
    switch (type) {
    case TokenType::INT_2:
    case TokenType::UINT_2:
    case TokenType::UZINT_2: return Base::BINARY;
    case TokenType::INT_8:
    case TokenType::UINT_8:
    case TokenType::UZINT_8: return Base::OCTAL;
    case TokenType::INT_10:
    case TokenType::UINT_10:
    case TokenType::UZINT_10: return Base::DECIMAL;
    case TokenType::INT_16:
    case TokenType::UINT_16:
    case TokenType::UZINT_16: return Base::HEXADECIMAL;
    default: std::unreachable();
    }
}

auto miscFromChar(byte c) noexcept -> std::optional<TokenType> {
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
    default: return std::nullopt;
    }
}

auto isSignedInt(TokenType t) noexcept -> bool {
    return TokenType::INT_2 <= t && t <= TokenType::INT_16;
}

auto isUnsignedInt(TokenType t) noexcept -> bool {
    return TokenType::UINT_2 <= t && t <= TokenType::UINT_16;
}

auto isSizeInt(TokenType t) noexcept -> bool {
    return TokenType::UZINT_2 <= t && t <= TokenType::UZINT_16;
}

auto isInt(TokenType t) noexcept -> bool {
    return isSignedInt(t) || isUnsignedInt(t) || isSizeInt(t);
}
} // namespace token_type

auto Token::promote() const -> Expected<std::string, TokenError> {
    if (type != TokenType::STRING && type != TokenType::MULTILINE_STRING) {
        return Unexpected{TokenError::NON_STRING_TOKEN};
    }

    // Here we can just trim off the start and finish of the string
    if (type == TokenType::STRING) {
        if (slice.size() < 2) { return Unexpected{TokenError::UNEXPECTED_CHAR}; }
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

auto Token::primitive() const noexcept -> bool {
    return std::ranges::contains(ALL_PRIMITIVES, type);
}
