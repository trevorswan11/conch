#include "syntax/token.hh"

#include <cctype>
#include <string>

#include "syntax/token_type.hh"

#include "assert.hh"
#include "types.hh"

namespace ghoti::syntax {

auto Token::materialize_string() const -> std::string {
    ASSERT(type == TokenType::STRING || type == TokenType::MULTILINE_STRING);

    // Here we can just trim off the start and finish of the string
    if (type == TokenType::STRING) { return std::string{slice.begin() + 1, slice.end() - 1}; }

    std::string builder{};
    builder.reserve(slice.size());

    auto at_line_start = true;
    for (usize i = 0; i < slice.size(); ++i) {
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

auto Token::is_decl_token() const noexcept -> bool {
    switch (type) {
    case TokenType::VAR:
    case TokenType::CONSTANT:
    case TokenType::CONSTEXPR:
    case TokenType::PUBLIC:
    case TokenType::EXTERN:
    case TokenType::EXPORT:    return true;
    default:                   return false;
    }
}

auto Token::is_member_token() const noexcept -> bool {
    switch (type) {
    case TokenType::IMPORT:
    case TokenType::USING:  return true;
    default:                return is_decl_token();
    }
}

} // namespace ghoti::syntax
