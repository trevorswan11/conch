#pragma once

#include <algorithm>
#include <array>
#include <utility>

#include "optional.hpp"
#include "types.hpp"

#include "lexer/token.hpp"

namespace conch {

enum class Precedence : u8 {
    LOWEST,
    BOOL_AND_OR,
    BOOL_EQUIV,
    BOOL_LT_GT,
    ADD_SUB,
    MUL_DIV,
    EXPONENT,
    PREFIX,
    RANGE,
    ASSIGNMENT,
    SCOPE_RESOLUTION,
    GROUP_CALL_IDX,
};

using Binding = std::pair<TokenType, Precedence>;

constexpr auto ALL_BINDINGS = []() {
    auto all_bindings = std::to_array<Binding>({
        {TokenType::PLUS, Precedence::ADD_SUB},
        {TokenType::MINUS, Precedence::ADD_SUB},
        {TokenType::STAR, Precedence::MUL_DIV},
        {TokenType::SLASH, Precedence::MUL_DIV},
        {TokenType::PERCENT, Precedence::MUL_DIV},
        {TokenType::STAR_STAR, Precedence::EXPONENT},
        {TokenType::BOOLEAN_AND, Precedence::BOOL_AND_OR},
        {TokenType::BOOLEAN_OR, Precedence::BOOL_AND_OR},
        {TokenType::EQ, Precedence::BOOL_EQUIV},
        {TokenType::NEQ, Precedence::BOOL_EQUIV},
        {TokenType::LT, Precedence::BOOL_LT_GT},
        {TokenType::LT_EQ, Precedence::BOOL_LT_GT},
        {TokenType::GT, Precedence::BOOL_LT_GT},
        {TokenType::GT_EQ, Precedence::BOOL_LT_GT},
        {TokenType::BW_AND, Precedence::MUL_DIV},
        {TokenType::BW_OR, Precedence::ADD_SUB},
        {TokenType::XOR, Precedence::ADD_SUB},
        {TokenType::SHR, Precedence::MUL_DIV},
        {TokenType::SHL, Precedence::MUL_DIV},
        {TokenType::LPAREN, Precedence::GROUP_CALL_IDX},
        {TokenType::LBRACKET, Precedence::GROUP_CALL_IDX},
        {TokenType::DOT_DOT, Precedence::RANGE},
        {TokenType::DOT_DOT_EQ, Precedence::RANGE},
        {TokenType::ASSIGN, Precedence::ASSIGNMENT},
        {TokenType::PLUS_ASSIGN, Precedence::ASSIGNMENT},
        {TokenType::MINUS_ASSIGN, Precedence::ASSIGNMENT},
        {TokenType::STAR_ASSIGN, Precedence::ASSIGNMENT},
        {TokenType::SLASH_ASSIGN, Precedence::ASSIGNMENT},
        {TokenType::PERCENT_ASSIGN, Precedence::ASSIGNMENT},
        {TokenType::BW_AND_ASSIGN, Precedence::ASSIGNMENT},
        {TokenType::BW_OR_ASSIGN, Precedence::ASSIGNMENT},
        {TokenType::SHL_ASSIGN, Precedence::ASSIGNMENT},
        {TokenType::SHR_ASSIGN, Precedence::ASSIGNMENT},
        {TokenType::NOT_ASSIGN, Precedence::ASSIGNMENT},
        {TokenType::XOR_ASSIGN, Precedence::ASSIGNMENT},
        {TokenType::DOT, Precedence::SCOPE_RESOLUTION},
        {TokenType::COLON_COLON, Precedence::SCOPE_RESOLUTION},
    });

    std::ranges::sort(all_bindings, {}, &Binding::first);
    return all_bindings;
}();

constexpr auto get_binding(TokenType tt) noexcept -> Optional<Binding> {
    const auto it = std::ranges::lower_bound(ALL_BINDINGS, tt, {}, &Binding::first);
    if (it == ALL_BINDINGS.end() || it->first != tt) { return nullopt; }
    return Optional<Binding>{*it};
}

} // namespace conch
