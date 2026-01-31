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
                                       std::vector<FunctionParameter> parameters,
                                       Box<TypeExpression>            return_type,
                                       Optional<Box<BlockStatement>>  body) noexcept
    : Expression{start_token, NodeKind::FUNCTION_EXPRESSION}, parameters_{std::move(parameters)},
      return_type_{std::move(return_type)}, body_{std::move(body)} {}

FunctionExpression::~FunctionExpression() = default;

auto FunctionExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto FunctionExpression::parse(Parser& parser)
    -> Expected<Box<FunctionExpression>, ParserDiagnostic> {
    TODO(parser);
}

} // namespace conch::ast
