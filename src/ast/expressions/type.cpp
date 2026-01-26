#include <utility>

#include "ast/expressions/type.hpp"

#include "ast/expressions/function.hpp"
#include "ast/expressions/identifier.hpp"

#include "visitor/visitor.hpp"

namespace ast {

ExplicitArrayType::ExplicitArrayType(std::vector<size_t>             dimensions,
                                     std::unique_ptr<TypeExpression> inner_type) noexcept
    : dimensions_{std::move(dimensions)}, inner_type_{std::move(inner_type)} {}

ExplicitArrayType::~ExplicitArrayType() = default;

ExplicitType::ExplicitType(ExplicitTypeVariant type, bool nullable) noexcept
    : ExplicitType{std::move(type), nullable, nullopt} {};

ExplicitType::ExplicitType(ExplicitTypeVariant              type,
                           bool                             nullable,
                           Optional<ExplicitTypeConstraint> constraint) noexcept
    : type_{std::move(type)}, nullable_{nullable}, constraint_{std::move(constraint)} {}

ExplicitType::~ExplicitType() = default;

TypeExpression::TypeExpression(const Token& start_token) noexcept
    : TypeExpression{start_token, nullopt} {}

TypeExpression::TypeExpression(const Token& start_token, Optional<ExplicitType> exp) noexcept
    : Expression{start_token}, explicit_{std::move(exp)} {}

TypeExpression::~TypeExpression() = default;

auto TypeExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto TypeExpression::parse(Parser& parser)
    -> Expected<std::unique_ptr<TypeExpression>, ParserDiagnostic> {
    TODO(parser);
}

} // namespace ast
