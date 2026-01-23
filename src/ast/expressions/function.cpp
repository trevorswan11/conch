#include <utility>

#include "ast/expressions/function.hpp"

#include "ast/expressions/identifier.hpp"
#include "ast/expressions/type.hpp"
#include "ast/statements/block.hpp"

#include "visitor/visitor.hpp"

namespace ast {

FunctionParameter::FunctionParameter(bool                                  r,
                                     std::unique_ptr<IdentifierExpression> n,
                                     std::unique_ptr<TypeExpression>       t) noexcept
    : FunctionParameter{r, std::move(n), std::move(t), nullopt} {}

FunctionParameter::FunctionParameter(bool                                  r,
                                     std::unique_ptr<IdentifierExpression> n,
                                     std::unique_ptr<TypeExpression>       t,
                                     Optional<std::unique_ptr<Expression>> d) noexcept
    : reference{r}, name{std::move(n)}, type{std::move(t)}, default_value{std::move(d)} {}

FunctionParameter::~FunctionParameter() = default;

FunctionExpression::FunctionExpression(const Token&                              start_token,
                                       std::vector<FunctionParameter>            parameters,
                                       std::unique_ptr<TypeExpression>           return_type,
                                       Optional<std::unique_ptr<BlockStatement>> body) noexcept
    : Expression{start_token}, parameters_{std::move(parameters)},
      return_type_{std::move(return_type)}, body_{std::move(body)} {}

FunctionExpression::~FunctionExpression() = default;

auto FunctionExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto FunctionExpression::parse(Parser& parser)
    -> Expected<std::unique_ptr<FunctionExpression>, ParserDiagnostic> {
    TODO(parser);
}

} // namespace ast
