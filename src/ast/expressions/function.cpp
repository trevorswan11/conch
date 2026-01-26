#include <utility>

#include "ast/expressions/function.hpp"

#include "ast/expressions/identifier.hpp"
#include "ast/expressions/type.hpp"
#include "ast/statements/block.hpp"

#include "visitor/visitor.hpp"

namespace ast {

FunctionParameter::FunctionParameter(bool                                  reference,
                                     std::unique_ptr<IdentifierExpression> name,
                                     std::unique_ptr<TypeExpression>       type) noexcept
    : FunctionParameter{reference, std::move(name), std::move(type), nullopt} {}

FunctionParameter::FunctionParameter(bool                                  reference,
                                     std::unique_ptr<IdentifierExpression> name,
                                     std::unique_ptr<TypeExpression>       type,
                                     Optional<std::unique_ptr<Expression>> default_value) noexcept
    : reference_{reference}, name_{std::move(name)}, type_{std::move(type)},
      default_value_{std::move(default_value)} {}

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
