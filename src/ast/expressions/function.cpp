#include <algorithm>
#include <utility>

#include "ast/expressions/function.hpp"

#include "ast/expressions/identifier.hpp"
#include "ast/expressions/type.hpp"
#include "ast/statements/block.hpp"

#include "visitor/visitor.hpp"

namespace conch::ast {

FunctionParameter::FunctionParameter(bool                      reference,
                                     Box<IdentifierExpression> name,
                                     Box<TypeExpression>       type,
                                     Optional<Box<Expression>> default_value) noexcept
    : reference_{reference}, name_{std::move(name)}, type_{std::move(type)},
      default_value_{std::move(default_value)} {}

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
    TODO(parser);
}

auto FunctionExpression::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted = as<FunctionExpression>(other);
    const auto  parameters_eq =
        std::ranges::equal(parameters_, casted.parameters_, [](const auto& a, const auto& b) {
            return a.reference_ == b.reference_ && *a.name_ == *b.name_ && *a.type_ == *b.type_ &&
                   optional::unsafe_eq<Expression>(a.default_value_, b.default_value_);
        });
    return parameters_eq && *return_type_ == *casted.return_type_ &&
           optional::unsafe_eq<BlockStatement>(body_, casted.body_);
}

} // namespace conch::ast
