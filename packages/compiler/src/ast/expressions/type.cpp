#include "ast/expressions/type.hpp"

#include "ast/expressions/function.hpp"
#include "ast/expressions/identifier.hpp"

#include "ast/visitor.hpp"

namespace conch::ast {

ExplicitArrayType::ExplicitArrayType(Box<Expression>   dimension,
                                     Box<ExplicitType> inner_type) noexcept
    : dimension_{std::move(dimension)}, inner_type_{std::move(inner_type)} {}

ExplicitArrayType::~ExplicitArrayType() = default;

ExplicitType::ExplicitType(TypeModifier modifier, ExplicitTypeVariant type) noexcept
    : modifier_{std::move(modifier)}, type_{std::move(type)} {}

ExplicitType::~ExplicitType() = default;

[[nodiscard]] auto ExplicitType::parse(Parser& parser) -> Expected<ExplicitType, ParserDiagnostic> {
    const auto start_token = parser.current_token();

    // Always check for a modifier and advance past it if present
    const auto modifier = TypeModifier::from_token(start_token);
    if (!modifier.is_value()) { parser.advance(); }

    // The array dimension of a type are only present conditionally
    if (parser.peek_token_is(TokenType::LBRACKET)) {
        parser.advance();
        auto dimension = TRY(parser.parse_expression());
        TRY(parser.expect_peek(TokenType::RBRACKET));

        // Arrays are recursively defined
        return ExplicitType{
            std::move(modifier),
            ExplicitArrayType{std::move(dimension), make_box<ExplicitType>(TRY(parse(parser)))}};
    } else if (!TypeModifier::from_token(start_token).is_value()) {
        // Don't advance since the parser does it implicitly here (costs two from_token calls)
        return ExplicitType{std::move(modifier), make_box<ExplicitType>(TRY(parse(parser)))};
    }

    // Otherwise the type has to be a 'simple' function or ident
    return TRY(([&]() -> Expected<ExplicitType, ParserDiagnostic> {
        const auto& peek_token = parser.peek_token();
        if (peek_token.is_primitive() || peek_token.type == TokenType::IDENT) {
            parser.advance();
            return ExplicitType{
                modifier,
                Node::downcast<IdentifierExpression>(TRY(IdentifierExpression::parse(parser)))};
        }

        // Otherwise the inner type must be a function
        const auto type_start = parser.current_token();
        parser.advance();
        auto type_expr = TRY(parser.parse_expression());

        if (type_expr->is<FunctionExpression>()) {
            auto function = Node::downcast<FunctionExpression>(std::move(type_expr));
            if (!modifier.is_value()) {
                return make_parser_unexpected(ParserError::ILLEGAL_TYPE_MODIFIER, type_start);
            }

            // Function types cannot have bodies
            if (function->has_body()) {
                return make_parser_unexpected(ParserError::EXPLICIT_FN_TYPE_HAS_BODY, type_start);
            }
            return ExplicitType{modifier, std::move(function)};
        }

        // No other expressions qualify as types
        return make_parser_unexpected(ParserError::ILLEGAL_EXPLICIT_TYPE, type_start);
    }()));
}

auto ExplicitType::is_equal(const ExplicitType& other) const noexcept -> bool {
    const auto& other_type = other.type_;
    if (type_.index() != other_type.index()) { return false; }
    return modifier_ == other.modifier_ &&
           std::visit(Overloaded{
                          [&other_type](const ExplicitIdentType& v) {
                              return *v == *std::get<ExplicitIdentType>(other_type);
                          },
                          [&other_type](const ExplicitFunctionType& v) {
                              return *v == *std::get<ExplicitFunctionType>(other_type);
                          },
                          [&other_type](const ExplicitArrayType& v1) {
                              const auto& v2 = std::get<ExplicitArrayType>(other_type);
                              return *v1.dimension_ == *v2.dimension_ &&
                                     *v1.inner_type_ == *v2.inner_type_;
                          },
                          [&other_type](const Box<ExplicitType>& v1) {
                              return *v1 == *std::get<ExplicitRecursiveType>(other_type);
                          },
                      },
                      type_);
}

TypeExpression::TypeExpression(const Token& start_token, Optional<ExplicitType> exp) noexcept
    : ExprBase{start_token}, explicit_{std::move(exp)} {}

TypeExpression::~TypeExpression() = default;

auto TypeExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto TypeExpression::parse(Parser& parser)
    -> Expected<std::pair<Box<Expression>, bool>, ParserDiagnostic> {
    // The start start token is always offset as this is called irregularly
    const auto start_token = parser.peek_token();

    auto [type, initialized] =
        TRY(([&]() -> Expected<std::pair<Box<TypeExpression>, bool>, ParserDiagnostic> {
            if (parser.peek_token_is(TokenType::WALRUS)) {
                auto type = make_box<TypeExpression>(start_token, nullopt);
                parser.advance();
                return std::pair{std::move(type), true};
            } else if (parser.peek_token_is(TokenType::COLON)) {
                parser.advance();
                auto explicit_type = TRY(ExplicitType::parse(parser));
                auto type = make_box<TypeExpression>(start_token, std::move(explicit_type));
                if (parser.peek_token_is(TokenType::ASSIGN)) {
                    parser.advance();
                    return std::pair{std::move(type), true};
                }
                return std::pair{std::move(type), false};
            } else {
                return Unexpected{parser.peek_error(TokenType::COLON)};
            }
        }()));

    // Advance again to prepare for rhs
    if (initialized) { parser.advance(); }
    return std::pair{box_into<Expression>(std::move(type)), initialized};
}

auto TypeExpression::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted = as<TypeExpression>(other);
    return optional::safe_eq<ExplicitType>(explicit_, casted.explicit_);
}

} // namespace conch::ast
