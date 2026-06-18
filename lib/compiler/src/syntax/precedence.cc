#include "syntax/precedence.hh"

#include <utility>

#include <stdx/fixed/enum_map.hh>
#include <stdx/option.hh>

#include "syntax/token_type.hh"

namespace ghoti::syntax {

namespace {

// NOLINTBEGIN
using MapPair = std::pair<TokenType, stdx::Option<Binding>>;
constexpr Binding assignment_binding{Precedence::ASSIGNMENT, true};

constexpr auto ALL_BINDINGS{stdx::fixed::EnumMap<TokenType, stdx::Option<Binding>>::from(
    stdx::Option<Binding>{stdx::none},
    MapPair{TokenType::PLUS, Precedence::ADD_SUB},
    MapPair{TokenType::MINUS, Precedence::ADD_SUB},
    MapPair{TokenType::STAR, Precedence::MUL_DIV},
    MapPair{TokenType::SLASH, Precedence::MUL_DIV},
    MapPair{TokenType::PERCENT, Precedence::MUL_DIV},
    MapPair{TokenType::BOOLEAN_AND, Precedence::BOOL_AND_OR},
    MapPair{TokenType::BOOLEAN_OR, Precedence::BOOL_AND_OR},
    MapPair{TokenType::EQ, Precedence::BOOL_EQUIV},
    MapPair{TokenType::NEQ, Precedence::BOOL_EQUIV},
    MapPair{TokenType::LT, Precedence::BOOL_LT_GT},
    MapPair{TokenType::LT_EQ, Precedence::BOOL_LT_GT},
    MapPair{TokenType::GT, Precedence::BOOL_LT_GT},
    MapPair{TokenType::GT_EQ, Precedence::BOOL_LT_GT},
    MapPair{TokenType::BW_AND, Precedence::MUL_DIV},
    MapPair{TokenType::BW_OR, Precedence::ADD_SUB},
    MapPair{TokenType::CARET, Precedence::ADD_SUB},
    MapPair{TokenType::SHR, Precedence::MUL_DIV},
    MapPair{TokenType::SHL, Precedence::MUL_DIV},
    MapPair{TokenType::LPAREN, Precedence::GROUP_CALL_IDX},
    MapPair{TokenType::LBRACKET, Precedence::GROUP_CALL_IDX},
    MapPair{TokenType::DOT_DOT, Precedence::RANGE},
    MapPair{TokenType::DOT_DOT_EQ, Precedence::RANGE},
    MapPair{TokenType::ASSIGN, assignment_binding},
    MapPair{TokenType::PLUS_ASSIGN, assignment_binding},
    MapPair{TokenType::MINUS_ASSIGN, assignment_binding},
    MapPair{TokenType::STAR_ASSIGN, assignment_binding},
    MapPair{TokenType::SLASH_ASSIGN, assignment_binding},
    MapPair{TokenType::PERCENT_ASSIGN, assignment_binding},
    MapPair{TokenType::BW_AND_ASSIGN, assignment_binding},
    MapPair{TokenType::BW_OR_ASSIGN, assignment_binding},
    MapPair{TokenType::SHL_ASSIGN, assignment_binding},
    MapPair{TokenType::SHR_ASSIGN, assignment_binding},
    MapPair{TokenType::NOT_ASSIGN, assignment_binding},
    MapPair{TokenType::XOR_ASSIGN, assignment_binding},
    MapPair{TokenType::DOT, Precedence::SCOPE_RESOLUTION},
    MapPair{TokenType::COLON_COLON, Precedence::SCOPE_RESOLUTION},
    MapPair{TokenType::LBRACE, Precedence::INITIALIZATION},
    MapPair{TokenType::COLON, Precedence::LABEL})};
// NOLINTEND

} // namespace

auto Binding::try_get_from(TokenType tt) noexcept -> stdx::Option<Binding> {
    return ALL_BINDINGS[tt];
}

} // namespace ghoti::syntax
