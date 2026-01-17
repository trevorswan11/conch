#include <expected>
#include <string>
#include <utility>

#include "lexer/token.hpp"

namespace token_type {
auto intoIntBase(TokenType type) -> Base {
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

auto miscFromChar(char c) -> std::optional<TokenType> {
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

auto isSignedInt(TokenType t) -> bool { return TokenType::INT_2 <= t && t <= TokenType::INT_16; }

auto isUnsignedInt(TokenType t) -> bool {
    return TokenType::UINT_2 <= t && t <= TokenType::UINT_16;
}

auto isSizeInt(TokenType t) -> bool { return TokenType::UZINT_2 <= t && t <= TokenType::UZINT_16; }

auto isInt(TokenType t) -> bool { return isSignedInt(t) || isUnsignedInt(t) || isSizeInt(t); }
} // namespace token_type

auto Token::promote() const -> std::expected<std::string, TokenError> {
    if (type != TokenType::STRING && type != TokenType::MULTILINE_STRING) {
        return std::unexpected{TokenError::NON_STRING_TOKEN};
    }

    std::string builder;
    builder.reserve(slice.size());

    if (type == TokenType::STRING) {
        if (slice.size() < 2) { return std::unexpected{TokenError::UNEXPECTED_CHAR}; }

        // Here we can just trim off the start and finish of the string
        if (slice.size() > 2) {
            const auto start = slice.begin() + 1;
            const auto end   = slice.end() - 1;

            builder += std::string_view{start, end};
        }
    } else if (type == TokenType::MULTILINE_STRING && !slice.empty()) {
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

            builder += c;
            if (c == '\n') { at_line_start = true; }
        }
    }

    return builder;
}
