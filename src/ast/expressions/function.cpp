#include <utility>

#include "ast/expressions/function.hpp"

#include "ast/expressions/identifier.hpp"
#include "ast/expressions/primitive.hpp" // IWYU pragma: keep
#include "ast/expressions/type.hpp"
#include "ast/statements/block.hpp"

#include "visitor/visitor.hpp"

namespace conch::ast {

FunctionParameter::FunctionParameter(Box<IdentifierExpression> name,
                                     Box<TypeExpression>       type) noexcept
    : name_{std::move(name)}, type_{std::move(type)} {}

FunctionParameter::~FunctionParameter() = default;

FunctionExpression::FunctionExpression(const Token&                   start_token,
                                       bool                           mut,
                                       std::vector<FunctionParameter> parameters,
                                       Box<TypeExpression>            return_type,
                                       Optional<Box<BlockStatement>>  body) noexcept
    : ExprBase{start_token}, mutable_{mut}, parameters_{std::move(parameters)},
      return_type_{std::move(return_type)}, body_{std::move(body)} {}

FunctionExpression::~FunctionExpression() = default;

auto FunctionExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto FunctionExpression::parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic> {
    const auto start_token = parser.current_token();

    // mut fn is valid syntax but fn mut is invalid
    const auto is_mut = start_token.type == TokenType::MUT;
    if (parser.current_token_is(TokenType::MUT)) {
        TRY(parser.expect_peek(TokenType::FUNCTION));
    } else if (parser.peek_token_is(TokenType::MUT)) {
        parser.advance();
        return make_parser_unexpected(ParserError::ILLEGAL_FUNCTION_DEFINITION, start_token);
    }
    assert(parser.current_token_is(TokenType::FUNCTION));

    // Parse the definition now that we're at the fn token
    std::vector<FunctionParameter> parameters;
    TRY(parser.expect_peek(TokenType::LPAREN));
    if (parser.peek_token_is(TokenType::RPAREN)) {
        parser.advance();
    } else {
        while (!parser.peek_token_is(TokenType::RPAREN) && !parser.peek_token_is(TokenType::END)) {
            parser.advance();
            auto name = downcast<IdentifierExpression>(TRY(IdentifierExpression::parse(parser)));
            auto [type_expr, initialized] = TRY(TypeExpression::parse(parser));
            auto type                     = downcast<TypeExpression>(std::move(type_expr));

            // There are no default values for parameters, and they must be explicitly typed
            if (initialized || !type->has_explicit_type()) {
                return make_parser_unexpected(ParserError::ILLEGAL_FUNCTION_PARAMETER_TYPE,
                                              type->get_token());
            }

            parameters.emplace_back(std::move(name), std::move(type));
            if (!parser.peek_token_is(TokenType::RPAREN)) {
                TRY(parser.expect_peek(TokenType::COMMA));
            }
        }
        TRY(parser.expect_peek(TokenType::RPAREN));
    }

    TRY(parser.expect_peek(TokenType::COLON));
    auto explicit_type = TRY(ExplicitType::parse(parser));
    auto return_type   = make_box<TypeExpression>(start_token, std::move(explicit_type));

    // If there is opening brace then just return without a body
    if (!parser.peek_token_is(TokenType::LBRACE)) {
        return make_box<FunctionExpression>(
            start_token, is_mut, std::move(parameters), std::move(return_type), nullopt);
    }

    // Otherwise there must be a well-formed block
    TRY(parser.expect_peek(TokenType::LBRACE));
    auto body = downcast<BlockStatement>(TRY(BlockStatement::parse(parser)));
    return make_box<FunctionExpression>(
        start_token, is_mut, std::move(parameters), std::move(return_type), std::move(body));
}

auto FunctionExpression::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted = as<FunctionExpression>(other);
    const auto  parameters_eq =
        std::ranges::equal(parameters_, casted.parameters_, [](const auto& a, const auto& b) {
            return *a.name_ == *b.name_ && *a.type_ == *b.type_;
        });
    return parameters_eq && *return_type_ == *casted.return_type_ &&
           optional::unsafe_eq<BlockStatement>(body_, casted.body_);
}

} // namespace conch::ast
