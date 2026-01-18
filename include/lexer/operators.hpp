#pragma once

#include <algorithm>
#include <array>
#include <string_view>
#include <utility>

#include "core.hpp"

#include "lexer/token.hpp"

using Operator    = std::pair<std::string_view, TokenType>;
using OperatorMap = flat_map_from_pair<Operator>;

namespace operators {
inline constexpr Operator ASSIGN{"=", TokenType::ASSIGN};
inline constexpr Operator WALRUS{":=", TokenType::WALRUS};
inline constexpr Operator PLUS{"+", TokenType::PLUS};
inline constexpr Operator PLUS_ASSIGN{"+=", TokenType::PLUS_ASSIGN};
inline constexpr Operator MINUS{"-", TokenType::MINUS};
inline constexpr Operator MINUS_ASSIGN{"-=", TokenType::MINUS_ASSIGN};
inline constexpr Operator STAR{"*", TokenType::STAR};
inline constexpr Operator STAR_ASSIGN{"*=", TokenType::STAR_ASSIGN};
inline constexpr Operator STAR_STAR{"**", TokenType::STAR_STAR};
inline constexpr Operator SLASH{"/", TokenType::SLASH};
inline constexpr Operator SLASH_ASSIGN{"/=", TokenType::SLASH_ASSIGN};
inline constexpr Operator PERCENT{"%", TokenType::PERCENT};
inline constexpr Operator PERCENT_ASSIGN{"%=", TokenType::PERCENT_ASSIGN};
inline constexpr Operator BANG{"!", TokenType::BANG};
inline constexpr Operator WHAT{"?", TokenType::WHAT};

inline constexpr Operator AND{"&", TokenType::AND};
inline constexpr Operator AND_ASSIGN{"&=", TokenType::AND_ASSIGN};
inline constexpr Operator OR{"|", TokenType::OR};
inline constexpr Operator OR_ASSIGN{"|=", TokenType::OR_ASSIGN};
inline constexpr Operator SHL{"<<", TokenType::SHL};
inline constexpr Operator SHL_ASSIGN{"<<=", TokenType::SHL_ASSIGN};
inline constexpr Operator SHR{">>", TokenType::SHR};
inline constexpr Operator SHR_ASSIGN{">>=", TokenType::SHR_ASSIGN};
inline constexpr Operator NOT{"~", TokenType::NOT};
inline constexpr Operator NOT_ASSIGN{"~=", TokenType::NOT_ASSIGN};
inline constexpr Operator XOR{"^", TokenType::XOR};
inline constexpr Operator XOR_ASSIGN{"^=", TokenType::XOR_ASSIGN};

inline constexpr Operator LT{"<", TokenType::LT};
inline constexpr Operator LTEQ{"<=", TokenType::LTEQ};
inline constexpr Operator GT{">", TokenType::GT};
inline constexpr Operator GTEQ{">=", TokenType::GTEQ};
inline constexpr Operator EQ{"==", TokenType::EQ};
inline constexpr Operator NEQ{"!=", TokenType::NEQ};

inline constexpr Operator COLON_COLON{"::", TokenType::COLON_COLON};
inline constexpr Operator DOT{".", TokenType::DOT};
inline constexpr Operator DOT_DOT{"..", TokenType::DOT_DOT};
inline constexpr Operator DOT_DOT_EQ{"..=", TokenType::DOT_DOT_EQ};
inline constexpr Operator FAT_ARROW{"=>", TokenType::FAT_ARROW};
inline constexpr Operator COMMENT{"//", TokenType::COMMENT};
inline constexpr Operator MULTILINE_STRING{"\\\\", TokenType::MULTILINE_STRING};
inline constexpr Operator REF{"ref", TokenType::REF};

inline constexpr auto RAW_OPERATORS = std::array{
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

inline constexpr auto MAX_OPERATOR_LEN = []() {
    return std::ranges::max_element(RAW_OPERATORS,
                                    [](auto a, auto b) { return a.first.size() < b.first.size(); })
        ->first.size();
}();
} // namespace operators

inline const OperatorMap ALL_OPERATORS{operators::RAW_OPERATORS.begin(),
                                       operators::RAW_OPERATORS.end()};
