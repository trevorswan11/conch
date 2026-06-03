#pragma once

#include <string>
#include <string_view>

#include "syntax/token_type.hh"

#include "diagnostic.hh"
#include "types.hh"

namespace ghoti {

namespace syntax {

struct Token {
    TokenType        type{};
    std::string_view slice{};
    usize            line{};
    usize            column{};

    Token() noexcept = default;
    Token(TokenType tt, std::string_view tok) noexcept : type{tt}, slice{tok} {}
    Token(TokenType tt, std::string_view slice, usize line, usize column) noexcept
        : type{tt}, slice{slice}, line{line}, column{column} {}

    explicit Token(const TypedIdentifier& tok) noexcept : type{tok.type}, slice{tok.name} {}

    // Materializes the token, asserting that it was a string token
    [[nodiscard]] auto materialize_string() const -> std::string;
    [[nodiscard]] auto is_decl_token() const noexcept -> bool;

    // Checks if the token can be used to kick off member parsing
    [[nodiscard]] auto is_member_token() const noexcept -> bool;

    // Check whether the token is an ident, primitive type, or builtin function.
    auto is_valid_ident() const noexcept -> bool { return token_type::is_valid_ident(type); }

    auto operator==(const Token& other) const noexcept -> bool {
        return type == other.type && slice == other.slice && line == other.line &&
               column == other.column;
    }
};

} // namespace syntax

namespace traits {

template <> struct SourceInfo<syntax::Token> {
    static auto get(const syntax::Token& t) -> SourceLocation { return {t.line, t.column}; }
};

} // namespace traits

} // namespace ghoti
