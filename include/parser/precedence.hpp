#pragma once

#include <algorithm>
#include <array>
#include <utility>

#include "util/optional.hpp"

#include "lexer/token.hpp"

enum class Precedence : u8 {
    LOWEST,
    BOOL_EQUIV,
    BOOL_LT_GT,
    ADD_SUB,
    MUL_DIV,
    EXPONENT,
    PREFIX,
    RANGE,
    ASSIGNMENT,
    SCOPE_RESOLUTION,
    CALL_IDX,
};

using Binding = std::pair<TokenType, Precedence>;

constexpr auto ALL_BINDINGS = std::to_array<Binding>({
    {TokenType::PLUS, Precedence::ADD_SUB},
    {TokenType::MINUS, Precedence::ADD_SUB},
    {TokenType::STAR, Precedence::MUL_DIV},
    {TokenType::SLASH, Precedence::MUL_DIV},
    {TokenType::PERCENT, Precedence::MUL_DIV},
    {TokenType::STAR_STAR, Precedence::EXPONENT},
    {TokenType::LT, Precedence::BOOL_LT_GT},
    {TokenType::LTEQ, Precedence::BOOL_LT_GT},
    {TokenType::GT, Precedence::BOOL_LT_GT},
    {TokenType::GTEQ, Precedence::BOOL_LT_GT},
    {TokenType::EQ, Precedence::BOOL_EQUIV},
    {TokenType::NEQ, Precedence::BOOL_EQUIV},
    {TokenType::BOOLEAN_AND, Precedence::BOOL_EQUIV},
    {TokenType::BOOLEAN_OR, Precedence::BOOL_EQUIV},
    {TokenType::IS, Precedence::BOOL_EQUIV},
    {TokenType::IN, Precedence::BOOL_EQUIV},
    {TokenType::AND, Precedence::ADD_SUB},
    {TokenType::OR, Precedence::ADD_SUB},
    {TokenType::XOR, Precedence::ADD_SUB},
    {TokenType::SHR, Precedence::MUL_DIV},
    {TokenType::SHL, Precedence::MUL_DIV},
    {TokenType::LPAREN, Precedence::CALL_IDX},
    {TokenType::LBRACKET, Precedence::CALL_IDX},
    {TokenType::DOT_DOT, Precedence::RANGE},
    {TokenType::DOT_DOT_EQ, Precedence::RANGE},
    {TokenType::ASSIGN, Precedence::ASSIGNMENT},
    {TokenType::PLUS_ASSIGN, Precedence::ASSIGNMENT},
    {TokenType::MINUS_ASSIGN, Precedence::ASSIGNMENT},
    {TokenType::STAR_ASSIGN, Precedence::ASSIGNMENT},
    {TokenType::SLASH_ASSIGN, Precedence::ASSIGNMENT},
    {TokenType::PERCENT_ASSIGN, Precedence::ASSIGNMENT},
    {TokenType::AND_ASSIGN, Precedence::ASSIGNMENT},
    {TokenType::OR_ASSIGN, Precedence::ASSIGNMENT},
    {TokenType::SHL_ASSIGN, Precedence::ASSIGNMENT},
    {TokenType::SHR_ASSIGN, Precedence::ASSIGNMENT},
    {TokenType::NOT_ASSIGN, Precedence::ASSIGNMENT},
    {TokenType::XOR_ASSIGN, Precedence::ASSIGNMENT},
    {TokenType::DOT, Precedence::SCOPE_RESOLUTION},
    {TokenType::COLON_COLON, Precedence::SCOPE_RESOLUTION},
    {TokenType::ORELSE, Precedence::ASSIGNMENT},
});

constexpr auto get_binding(TokenType tt) noexcept -> Optional<Binding> {
    const auto it = std::ranges::find(ALL_BINDINGS, tt, &Binding::first);
    return it == ALL_BINDINGS.end() ? nullopt : Optional<Binding>{*it};
}
