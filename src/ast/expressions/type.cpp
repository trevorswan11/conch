#include <utility>

#include "ast/expressions/type.hpp"

#include "ast/expressions/function.hpp"
#include "ast/expressions/identifier.hpp"

#include "visitor/visitor.hpp"

namespace ast {

ExplicitArrayType::ExplicitArrayType(std::vector<size_t>             d,
                                     std::unique_ptr<TypeExpression> i) noexcept
    : dimensions{std::move(d)}, inner_type{std::move(i)} {}

ExplicitArrayType::~ExplicitArrayType() = default;

ExplicitType::ExplicitType(ExplicitTypeVariant t, bool n) noexcept
    : ExplicitType{std::move(t), n, std::nullopt} {};

ExplicitType::ExplicitType(ExplicitTypeVariant                   t,
                           bool                                  n,
                           std::optional<ExplicitTypeConstraint> c) noexcept
    : type{std::move(t)}, nullable{n}, constraint{std::move(c)} {}

ExplicitType::~ExplicitType() = default;

TypeExpression::TypeExpression(const Token& start_token) noexcept
    : TypeExpression{start_token, std::nullopt} {}

TypeExpression::TypeExpression(const Token& start_token, std::optional<ExplicitType> exp) noexcept
    : Expression{start_token}, explicit_{std::move(exp)} {}

TypeExpression::~TypeExpression() = default;

auto TypeExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto TypeExpression::parse(Parser& parser)
    -> Expected<std::unique_ptr<TypeExpression>, ParserDiagnostic> {
    TODO(parser);
}

} // namespace ast
