#include "syntax/precedence.hh"

#include "syntax/token_type.hh"

#include <fixed/enum_map.hh>
#include <option.hh>

namespace ghoti::syntax {

namespace {

constexpr auto ALL_BINDINGS = [] -> auto {
    fixed::EnumMap<TokenType, opt::Option<Binding>> bindings;

    // NOLINTBEGIN
    bindings[TokenType::PLUS]           = {Precedence::ADD_SUB};
    bindings[TokenType::MINUS]          = {Precedence::ADD_SUB};
    bindings[TokenType::STAR]           = {Precedence::MUL_DIV};
    bindings[TokenType::SLASH]          = {Precedence::MUL_DIV};
    bindings[TokenType::PERCENT]        = {Precedence::MUL_DIV};
    bindings[TokenType::BOOLEAN_AND]    = {Precedence::BOOL_AND_OR};
    bindings[TokenType::BOOLEAN_OR]     = {Precedence::BOOL_AND_OR};
    bindings[TokenType::EQ]             = {Precedence::BOOL_EQUIV};
    bindings[TokenType::NEQ]            = {Precedence::BOOL_EQUIV};
    bindings[TokenType::LT]             = {Precedence::BOOL_LT_GT};
    bindings[TokenType::LT_EQ]          = {Precedence::BOOL_LT_GT};
    bindings[TokenType::GT]             = {Precedence::BOOL_LT_GT};
    bindings[TokenType::GT_EQ]          = {Precedence::BOOL_LT_GT};
    bindings[TokenType::BW_AND]         = {Precedence::MUL_DIV};
    bindings[TokenType::BW_OR]          = {Precedence::ADD_SUB};
    bindings[TokenType::CARET]          = {Precedence::ADD_SUB};
    bindings[TokenType::SHR]            = {Precedence::MUL_DIV};
    bindings[TokenType::SHL]            = {Precedence::MUL_DIV};
    bindings[TokenType::LPAREN]         = {Precedence::GROUP_CALL_IDX};
    bindings[TokenType::LBRACKET]       = {Precedence::GROUP_CALL_IDX};
    bindings[TokenType::DOT_DOT]        = {Precedence::RANGE};
    bindings[TokenType::DOT_DOT_EQ]     = {Precedence::RANGE};
    bindings[TokenType::ASSIGN]         = {Precedence::ASSIGNMENT, true};
    bindings[TokenType::PLUS_ASSIGN]    = {Precedence::ASSIGNMENT, true};
    bindings[TokenType::MINUS_ASSIGN]   = {Precedence::ASSIGNMENT, true};
    bindings[TokenType::STAR_ASSIGN]    = {Precedence::ASSIGNMENT, true};
    bindings[TokenType::SLASH_ASSIGN]   = {Precedence::ASSIGNMENT, true};
    bindings[TokenType::PERCENT_ASSIGN] = {Precedence::ASSIGNMENT, true};
    bindings[TokenType::BW_AND_ASSIGN]  = {Precedence::ASSIGNMENT, true};
    bindings[TokenType::BW_OR_ASSIGN]   = {Precedence::ASSIGNMENT, true};
    bindings[TokenType::SHL_ASSIGN]     = {Precedence::ASSIGNMENT, true};
    bindings[TokenType::SHR_ASSIGN]     = {Precedence::ASSIGNMENT, true};
    bindings[TokenType::NOT_ASSIGN]     = {Precedence::ASSIGNMENT, true};
    bindings[TokenType::XOR_ASSIGN]     = {Precedence::ASSIGNMENT, true};
    bindings[TokenType::DOT]            = {Precedence::SCOPE_RESOLUTION};
    bindings[TokenType::COLON_COLON]    = {Precedence::SCOPE_RESOLUTION};
    bindings[TokenType::LBRACE]         = {Precedence::INITIALIZATION};
    bindings[TokenType::COLON]          = {Precedence::LABEL};
    // NOLINTEND

    return bindings;
}();

} // namespace

auto Binding::try_get_from(TokenType tt) noexcept -> opt::Option<Binding> {
    return ALL_BINDINGS[tt];
}

} // namespace ghoti::syntax
