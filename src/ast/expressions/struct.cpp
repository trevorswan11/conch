#include <utility>

#include "ast/expressions/struct.hpp"

#include "ast/expressions/function.hpp"
#include "ast/expressions/identifier.hpp"
#include "ast/expressions/type.hpp"

#include "visitor/visitor.hpp"

namespace ast {

StructMember::StructMember(std::unique_ptr<IdentifierExpression> n,
                           std::unique_ptr<TypeExpression>       t) noexcept
    : StructMember{std::move(n), std::move(t), std::nullopt} {}

StructMember::StructMember(std::unique_ptr<IdentifierExpression>      n,
                           std::unique_ptr<TypeExpression>            t,
                           std::optional<std::unique_ptr<Expression>> d) noexcept
    : name{std::move(n)}, type{std::move(t)}, default_value{std::move(d)} {}

StructMember::~StructMember() = default;

StructExpression::StructExpression(const Token&                          start_token,
                                   std::unique_ptr<IdentifierExpression> name,
                                   std::vector<StructMember>             members) noexcept
    : Expression{start_token}, name_{std::move(name)}, members_{std::move(members)} {}

StructExpression::~StructExpression() = default;

auto StructExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto StructExpression::parse(Parser& parser)
    -> Expected<std::unique_ptr<StructExpression>, ParserDiagnostic> {
    TODO(parser);
}

} // namespace ast
