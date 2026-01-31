#include <utility>

#include "ast/statements/decl.hpp"

#include "ast/expressions/function.hpp"
#include "ast/expressions/identifier.hpp"
#include "ast/expressions/type.hpp"

#include "visitor/visitor.hpp"

namespace conch::ast {

DeclStatement::DeclStatement(const Token&              start_token,
                             Box<IdentifierExpression> ident,
                             Box<TypeExpression>       type,
                             Optional<Box<Expression>> value,
                             DeclModifiers             modifiers) noexcept
    : Statement{start_token, NodeKind::DECL_STATEMENT}, ident_{std::move(ident)},
      type_{std::move(type)}, value_{std::move(value)}, modifiers_{modifiers} {}

DeclStatement::~DeclStatement() = default;

auto DeclStatement::accept(Visitor& v) const -> void { v.visit(*this); }

auto DeclStatement::parse(Parser& parser) -> Expected<Box<DeclStatement>, ParserDiagnostic> {
    const auto    start_token = parser.current_token();
    DeclModifiers modifiers   = token_to_modifier(start_token).value();

    Optional<DeclModifiers> current_modifier;
    while ((current_modifier = token_to_modifier(parser.peek_token()))) {
        parser.advance();
        const auto& next_modifier = current_modifier.value();
        if (modifiers_has(modifiers, next_modifier)) {
            return make_parser_unexpected(ParserError::DUPLICATE_DECL_MODIFIER, start_token);
        }

        modifiers |= next_modifier;
    }

    if (!validate_modifiers(modifiers)) {
        return make_parser_unexpected(ParserError::ILLEGAL_DECL_MODIFIERS, start_token);
    }

    TRY(parser.expect_peek(TokenType::IDENT));
    auto decl_name                      = TRY(IdentifierExpression::parse(parser));
    auto [decl_type, value_initialized] = TRY(TypeExpression::parse(parser));

    Optional<Box<Expression>> decl_value;
    if (value_initialized) {
        if (!modifiers_has(modifiers, DeclModifiers::MUTABLE)) {
            return make_parser_unexpected(ParserError::CONST_DECL_MISSING_VALUE, start_token);
        }
        decl_value = TRY(parser.parse_expression());
    } else {
        if (!modifiers_has(modifiers, DeclModifiers::MUTABLE)) {
            return make_parser_unexpected(ParserError::CONST_DECL_MISSING_VALUE, start_token);
        } else if (!decl_type->has_explicit_type()) {
            return make_parser_unexpected(ParserError::FORWARD_VAR_DECL_MISSING_TYPE, start_token);
        }
    }

    return make_box<DeclStatement>(
        start_token, std::move(decl_name), std::move(decl_type), std::move(decl_value), modifiers);
}

} // namespace conch::ast
