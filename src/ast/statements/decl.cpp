#include <utility>

#include "ast/statements/decl.hpp"

#include "ast/expressions/function.hpp"
#include "ast/expressions/identifier.hpp"
#include "ast/expressions/type.hpp"

#include "visitor/visitor.hpp"

namespace ast {

DeclStatement::DeclStatement(const Token&                          start_token,
                             std::unique_ptr<IdentifierExpression> ident,
                             std::unique_ptr<TypeExpression>       type,
                             Optional<std::unique_ptr<Expression>> value,
                             bool                                  constant) noexcept
    : Statement{start_token}, ident_{std::move(ident)}, type_{std::move(type)},
      value_{std::move(value)}, constant_{constant} {}

DeclStatement::~DeclStatement() = default;

auto DeclStatement::accept(Visitor& v) const -> void { v.visit(*this); }

auto DeclStatement::parse(Parser& parser)
    -> Expected<std::unique_ptr<DeclStatement>, ParserDiagnostic> {
    const auto start_token = parser.current_token();
    TRY(parser.expect_peek(TokenType::IDENT));

    const auto constant_decl            = start_token.type == TokenType::CONST;
    auto       decl_name                = TRY(IdentifierExpression::parse(parser));
    auto [decl_type, value_initialized] = TRY(TypeExpression::parse(parser));

    Optional<std::unique_ptr<Expression>> decl_value;
    if (value_initialized) {
        if (constant_decl) {
            return make_parser_unexpected(ParserError::CONST_DECL_MISSING_VALUE, start_token);
        }
        decl_value = TRY(parser.parse_expression());
    } else {
        if (constant_decl) {
            return make_parser_unexpected(ParserError::CONST_DECL_MISSING_VALUE, start_token);
        } else if (!decl_type->explicit_type()) {
            return make_parser_unexpected(ParserError::FORWARD_VAR_DECL_MISSING_TYPE, start_token);
        }
    }

    return std::make_unique<DeclStatement>(start_token,
                                           std::move(decl_name),
                                           std::move(decl_type),
                                           std::move(decl_value),
                                           constant_decl);
}

} // namespace ast
