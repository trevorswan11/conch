#include <utility>

#include "ast/expressions/type.hpp"

#include "ast/expressions/function.hpp"
#include "ast/expressions/identifier.hpp"

#include "visitor/visitor.hpp"

namespace conch::ast {

ExplicitArrayType::ExplicitArrayType(std::vector<size_t> dimensions,
                                     Box<TypeExpression> inner_type) noexcept
    : dimensions_{std::move(dimensions)}, inner_type_{std::move(inner_type)} {}

ExplicitArrayType::~ExplicitArrayType() = default;

ExplicitType::ExplicitType(ExplicitTypeVariant              type,
                           bool                             nullable,
                           Optional<ExplicitTypeConstraint> constraint) noexcept
    : type_{std::move(type)}, nullable_{nullable}, constraint_{std::move(constraint)} {}

ExplicitType::~ExplicitType() = default;

TypeExpression::TypeExpression(const Token& start_token, Optional<ExplicitType> exp) noexcept
    : Expression{start_token, NodeKind::TYPE_EXPRESSION}, explicit_{std::move(exp)} {}

TypeExpression::~TypeExpression() = default;

auto TypeExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto TypeExpression::parse(Parser& parser)
    -> Expected<std::pair<Box<TypeExpression>, bool>, ParserDiagnostic> {
    TODO(parser);
}

auto TypeExpression::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted = as<TypeExpression>(other);
    return optional::safe_eq<ExplicitType>(
        explicit_, casted.explicit_, [](const auto& a, const auto& b) {
            if (a.type_.index() != b.type_.index()) { return false; }

            const auto& btype = b.type_;
            const auto  variant_eq =
                std::visit(overloaded{
                               [&btype](const ExplicitIdentType& v) {
                                   return *v == *std::get<ExplicitIdentType>(btype);
                               },
                               [&btype](const ExplicitReferredType& v) {
                                   return *v == *std::get<ExplicitReferredType>(btype);
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

            return variant_eq && a.nullable_ == b.nullable_ && a.constraint_ == b.constraint_;
        });
}

} // namespace conch::ast
