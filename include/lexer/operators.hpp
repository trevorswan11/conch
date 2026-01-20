#pragma once

#include <algorithm>
#include <array>
#include <optional>
#include <string_view>
#include <utility>

#include "lexer/token.hpp"

using Operator = std::pair<std::string_view, TokenType>;

namespace operators {
constexpr Operator ASSIGN{"=", TokenType::ASSIGN};
constexpr Operator WALRUS{":=", TokenType::WALRUS};
constexpr Operator PLUS{"+", TokenType::PLUS};
constexpr Operator PLUS_ASSIGN{"+=", TokenType::PLUS_ASSIGN};
constexpr Operator MINUS{"-", TokenType::MINUS};
constexpr Operator MINUS_ASSIGN{"-=", TokenType::MINUS_ASSIGN};
constexpr Operator STAR{"*", TokenType::STAR};
constexpr Operator STAR_ASSIGN{"*=", TokenType::STAR_ASSIGN};
constexpr Operator STAR_STAR{"**", TokenType::STAR_STAR};
constexpr Operator SLASH{"/", TokenType::SLASH};
constexpr Operator SLASH_ASSIGN{"/=", TokenType::SLASH_ASSIGN};
constexpr Operator PERCENT{"%", TokenType::PERCENT};
constexpr Operator PERCENT_ASSIGN{"%=", TokenType::PERCENT_ASSIGN};
constexpr Operator BANG{"!", TokenType::BANG};
constexpr Operator WHAT{"?", TokenType::WHAT};

constexpr Operator AND{"&", TokenType::AND};
constexpr Operator AND_ASSIGN{"&=", TokenType::AND_ASSIGN};
constexpr Operator OR{"|", TokenType::OR};
constexpr Operator OR_ASSIGN{"|=", TokenType::OR_ASSIGN};
constexpr Operator SHL{"<<", TokenType::SHL};
constexpr Operator SHL_ASSIGN{"<<=", TokenType::SHL_ASSIGN};
constexpr Operator SHR{">>", TokenType::SHR};
constexpr Operator SHR_ASSIGN{">>=", TokenType::SHR_ASSIGN};
constexpr Operator NOT{"~", TokenType::NOT};
constexpr Operator NOT_ASSIGN{"~=", TokenType::NOT_ASSIGN};
constexpr Operator XOR{"^", TokenType::XOR};
constexpr Operator XOR_ASSIGN{"^=", TokenType::XOR_ASSIGN};

constexpr Operator LT{"<", TokenType::LT};
constexpr Operator LTEQ{"<=", TokenType::LTEQ};
constexpr Operator GT{">", TokenType::GT};
constexpr Operator GTEQ{">=", TokenType::GTEQ};
constexpr Operator EQ{"==", TokenType::EQ};
constexpr Operator NEQ{"!=", TokenType::NEQ};

constexpr Operator COLON_COLON{"::", TokenType::COLON_COLON};
constexpr Operator DOT{".", TokenType::DOT};
constexpr Operator DOT_DOT{"..", TokenType::DOT_DOT};
constexpr Operator DOT_DOT_EQ{"..=", TokenType::DOT_DOT_EQ};
constexpr Operator FAT_ARROW{"=>", TokenType::FAT_ARROW};
constexpr Operator COMMENT{"//", TokenType::COMMENT};
constexpr Operator MULTILINE_STRING{"\\\\", TokenType::MULTILINE_STRING};
constexpr Operator REF{"ref", TokenType::REF};
} // namespace operators

constexpr auto ALL_OPERATORS = std::array{
    operators::ASSIGN,
    operators::WALRUS,
    operators::PLUS,
    operators::PLUS_ASSIGN,
    operators::MINUS,
    operators::MINUS_ASSIGN,
    operators::STAR,
    operators::STAR_ASSIGN,
    operators::STAR_STAR,
    operators::SLASH,
    operators::SLASH_ASSIGN,
    operators::PERCENT,
    operators::PERCENT_ASSIGN,
    operators::BANG,
    operators::WHAT,
    operators::AND,
    operators::AND_ASSIGN,
    operators::OR,
    operators::OR_ASSIGN,
    operators::SHL,
    operators::SHL_ASSIGN,
    operators::SHR,
    operators::SHR_ASSIGN,
    operators::NOT,
    operators::NOT_ASSIGN,
    operators::XOR,
    operators::XOR_ASSIGN,
    operators::LT,
    operators::LTEQ,
    operators::GT,
    operators::GTEQ,
    operators::EQ,
    operators::NEQ,
    operators::COLON_COLON,
    operators::DOT,
    operators::DOT_DOT,
    operators::DOT_DOT_EQ,
    operators::FAT_ARROW,
    operators::COMMENT,
    operators::MULTILINE_STRING,
    operators::REF,
};

constexpr auto MAX_OPERATOR_LEN = std::ranges::max_element(ALL_OPERATORS, [](auto a, auto b) {
                                      return a.first.size() < b.first.size();
                                  })->first.size();

constexpr auto get_operator(std::string_view sv) noexcept -> std::optional<Operator> {
    const auto it = std::ranges::find(ALL_OPERATORS, sv, &Operator::first);
    return it == ALL_OPERATORS.end() ? std::nullopt : std::optional{*it};
}
