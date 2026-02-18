#include <algorithm>

#include "ast/expressions/type.hpp"

#include "ast/expressions/function.hpp"
#include "ast/expressions/identifier.hpp"
#include "ast/expressions/primitive.hpp"

#include "ast/visitor.hpp"

#include "common.hpp"

namespace conch::ast {

ExplicitArrayType::ExplicitArrayType(std::vector<Box<USizeIntegerExpression>> dimensions,
                                     Box<TypeExpression>                      inner_type) noexcept
    : dimensions_{std::move(dimensions)}, inner_type_{std::move(inner_type)} {}

ExplicitArrayType::~ExplicitArrayType() = default;

ExplicitType::ExplicitType(const Optional<TypeModifiers>& modifiers,
                           ExplicitTypeVariant            type,
                           bool                           primitive) noexcept
    : modifiers_{modifiers}, type_{std::move(type)}, primitive_{primitive} {}

ExplicitType::~ExplicitType() = default;

[[nodiscard]] auto ExplicitType::parse(Parser& parser) -> Expected<ExplicitType, ParserDiagnostic> {
    const auto start_token = parser.current_token();

    // The modifiers are initialized only if the first token is one
    const auto parse_modifiers =
        [&parser]() -> Expected<Optional<TypeModifiers>, ParserDiagnostic> {
        Optional<TypeModifiers> modifiers;

        const auto local_start = parser.current_token();
        if (token_to_modifier(local_start)) { modifiers.emplace(); }
        while (const auto modifier = token_to_modifier(parser.current_token())) {
            parser.advance();
            *modifiers |= *modifier;
        }

        // The modifiers are invalid if they were initialized and are not unique
        if (modifiers && !validate_modifiers(*modifiers)) {
            return make_parser_unexpected(ParserError::ILLEGAL_TYPE_MODIFIERS, local_start);
        }
        return modifiers;
    };
    const auto outer_modifiers = TRY(parse_modifiers());

    // The array dimensions of a type are only present conditionally
    auto opt_array_dims = TRY(
        ([&]() -> Expected<Optional<std::vector<Box<USizeIntegerExpression>>>, ParserDiagnostic> {
            if (!parser.peek_token_is(TokenType::LBRACKET)) { return nullopt; }

            // Arrays are a little weird especially with the function signature
            parser.advance();
            std::vector<Box<USizeIntegerExpression>> dimensions;

            while (!parser.peek_token_is(TokenType::RBRACKET) &&
                   !parser.peek_token_is(TokenType::END)) {
                parser.advance();
                const auto integer_token = parser.current_token();
                if (!token_type::is_usize_int(integer_token.type)) {
                    return make_parser_unexpected(ParserError::UNEXPECTED_ARRAY_SIZE_TOKEN,
                                                  integer_token);
                }

                auto dim = Node::downcast<USizeIntegerExpression>(
                    TRY(USizeIntegerExpression::parse(parser)));
                if (dim->get_value() == 0) {
                    return make_parser_unexpected(ParserError::EMPTY_ARRAY, integer_token);
                }

                dimensions.emplace_back(std::move(dim));
                if (!parser.peek_token_is(TokenType::RBRACKET)) {
                    TRY(parser.expect_peek(TokenType::COMMA));
                }
            }

            TRY(parser.expect_peek(TokenType::RBRACKET));
            if (dimensions.empty()) {
                return make_parser_unexpected(ParserError::MISSING_ARRAY_SIZE_TOKEN, start_token);
            }
            return dimensions;
        }()));

    // Inner modifiers exist only if there is an outer array type, otherwise they mirror outer
    const auto inner_modifiers = TRY(([&]() -> Expected<Optional<TypeModifiers>, ParserDiagnostic> {
        if (opt_array_dims) { return parse_modifiers(); }
        return outer_modifiers;
    }()));

    auto inner_type = TRY(([&]() -> Expected<ExplicitType, ParserDiagnostic> {
        const auto is_primitive = parser.peek_token().is_primitive();
        if (is_primitive || parser.peek_token_is(TokenType::IDENT)) {
            parser.advance();
            return ExplicitType{
                inner_modifiers,
                Node::downcast<IdentifierExpression>(TRY(IdentifierExpression::parse(parser))),
                is_primitive};
        }

        // Otherwise the inner type must be a function
        const auto type_start = parser.current_token();
        parser.advance();
        auto type_expr = TRY(parser.parse_expression());

        if (type_expr->is<FunctionExpression>()) {
            auto function = Node::downcast<FunctionExpression>(std::move(type_expr));
            if (inner_modifiers) {
                return make_parser_unexpected(ParserError::ILLEGAL_TYPE_MODIFIERS, type_start);
            }

            // Function types cannot have bodies
            if (function->has_body()) {
                return make_parser_unexpected(ParserError::EXPLICIT_FN_TYPE_HAS_BODY, type_start);
            }
            return ExplicitType{nullopt, std::move(function), false};
        }

        // No other expressions qualify as types
        return make_parser_unexpected(ParserError::ILLEGAL_EXPLICIT_TYPE, type_start);
    }()));

    // If we parsed an array type at the start, then the actual type is nested
    if (opt_array_dims) {
        auto inner_type_expr = make_box<TypeExpression>(start_token, std::move(inner_type));
        ExplicitArrayType array_type{std::move(*opt_array_dims), std::move(inner_type_expr)};
        return ExplicitType{outer_modifiers, std::move(array_type), false};
    }
    return inner_type;
}

TypeExpression::TypeExpression(const Token& start_token, Optional<ExplicitType> exp) noexcept
    : ExprBase{start_token}, explicit_{std::move(exp)} {}

TypeExpression::~TypeExpression() = default;

auto TypeExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto TypeExpression::parse(Parser& parser)
    -> Expected<std::pair<Box<Expression>, bool>, ParserDiagnostic> {
    const auto start_token = parser.current_token();

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
    parser.advance();
    return std::pair{box_into<Expression>(std::move(type)), initialized};
}

auto TypeExpression::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted = as<TypeExpression>(other);
    return optional::safe_eq<ExplicitType>(
        explicit_, casted.explicit_, [](const auto& a, const auto& b) {
            if (a.type_.index() != b.type_.index()) { return false; }

            const auto& btype = b.type_;
            const auto  variant_eq =
                std::visit(Overloaded{
                               [&btype](const ExplicitIdentType& v) {
                                   return *v == *std::get<ExplicitIdentType>(btype);
                               },
                               [&btype](const ExplicitFunctionType& v) {
                                   return *v == *std::get<ExplicitFunctionType>(btype);
                               },
                               [&btype](const ExplicitArrayType& v1) {
                                   const auto& v2 = std::get<ExplicitArrayType>(btype);
                                   return std::ranges::equal(v1.dimensions_, v2.dimensions_) &&
                                          *v1.inner_type_ == *v2.inner_type_;
                               },
                           },
                           a.type_);

            return variant_eq && a.primitive_ == b.primitive_;
        });
}

} // namespace conch::ast
