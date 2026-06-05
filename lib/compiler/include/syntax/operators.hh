#pragma once

#include <string_view>

#include "syntax/token_type.hh"

#include "option.hh"

namespace ghoti::syntax {

using Operator = TypedIdentifier;

namespace operators {

constexpr Operator ASSIGN{"=", TokenType::ASSIGN};
constexpr Operator WALRUS{":=", TokenType::WALRUS};
constexpr Operator PLUS{"+", TokenType::PLUS};
constexpr Operator PLUS_ASSIGN{"+=", TokenType::PLUS_ASSIGN};
constexpr Operator MINUS{"-", TokenType::MINUS};
constexpr Operator MINUS_ASSIGN{"-=", TokenType::MINUS_ASSIGN};
constexpr Operator STAR{"*", TokenType::STAR};
constexpr Operator STAR_ASSIGN{"*=", TokenType::STAR_ASSIGN};
constexpr Operator SLASH{"/", TokenType::SLASH};
constexpr Operator SLASH_ASSIGN{"/=", TokenType::SLASH_ASSIGN};
constexpr Operator PERCENT{"%", TokenType::PERCENT};
constexpr Operator PERCENT_ASSIGN{"%=", TokenType::PERCENT_ASSIGN};
constexpr Operator BANG{"!", TokenType::BANG};
constexpr Operator AND_MUT{"&mut", TokenType::AND_MUT};
constexpr Operator CARET_MUT{"^mut", TokenType::CARET_MUT};

constexpr Operator BW_AND{"&", TokenType::BW_AND};
constexpr Operator BW_AND_ASSIGN{"&=", TokenType::BW_AND_ASSIGN};
constexpr Operator BW_OR{"|", TokenType::BW_OR};
constexpr Operator BW_OR_ASSIGN{"|=", TokenType::BW_OR_ASSIGN};
constexpr Operator SHL{"<<", TokenType::SHL};
constexpr Operator SHL_ASSIGN{"<<=", TokenType::SHL_ASSIGN};
constexpr Operator SHR{">>", TokenType::SHR};
constexpr Operator SHR_ASSIGN{">>=", TokenType::SHR_ASSIGN};
constexpr Operator NOT{"~", TokenType::NOT};
constexpr Operator NOT_ASSIGN{"~=", TokenType::NOT_ASSIGN};
constexpr Operator CARET{"^", TokenType::CARET};
constexpr Operator XOR_ASSIGN{"^=", TokenType::XOR_ASSIGN};

constexpr Operator BOOLEAN_AND{"and", TokenType::BOOLEAN_AND};
constexpr Operator BOOLEAN_OR{"or", TokenType::BOOLEAN_OR};
constexpr Operator LT{"<", TokenType::LT};
constexpr Operator LT_EQ{"<=", TokenType::LT_EQ};
constexpr Operator GT{">", TokenType::GT};
constexpr Operator GT_EQ{">=", TokenType::GT_EQ};
constexpr Operator EQ{"==", TokenType::EQ};
constexpr Operator NEQ{"!=", TokenType::NEQ};

constexpr Operator ELLIPSIS{"...", TokenType::ELLIPSIS};
constexpr Operator COLON_COLON{"::", TokenType::COLON_COLON};
constexpr Operator DOT{".", TokenType::DOT};
constexpr Operator DOT_DOT{"..", TokenType::DOT_DOT};
constexpr Operator DOT_DOT_EQ{"..=", TokenType::DOT_DOT_EQ};
constexpr Operator FAT_ARROW{"=>", TokenType::FAT_ARROW};
constexpr Operator COMMENT{"//", TokenType::COMMENT};
constexpr Operator MULTILINE_STRING{"\\\\", TokenType::MULTILINE_STRING};
constexpr Operator NULL_TERMINATED{":0", TokenType::NULL_TERMINATED};

} // namespace operators

[[nodiscard]] auto max_operator_length() noexcept -> usize;
[[nodiscard]] auto get_operator_opt(std::string_view sv) noexcept -> opt::Option<TokenType>;

} // namespace ghoti::syntax
