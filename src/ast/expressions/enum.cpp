#include <utility>

#include "ast/expressions/enum.hpp"

#include "ast/expressions/identifier.hpp"

#include "visitor/visitor.hpp"

namespace ast {

EnumVariant::EnumVariant(std::unique_ptr<IdentifierExpression> e) noexcept
    : EnumVariant{std::move(e), nullopt} {}

EnumVariant::EnumVariant(std::unique_ptr<IdentifierExpression> e,
                         Optional<std::unique_ptr<Expression>> v) noexcept
    : enumeration{std::move(e)}, value{std::move(v)} {}

EnumVariant::~EnumVariant() = default;

EnumExpression::EnumExpression(const Token&                          start_token,
                               std::unique_ptr<IdentifierExpression> name,
                               std::vector<EnumVariant>              variants) noexcept
    : Expression{start_token}, name_{std::move(name)}, variants_{std::move(variants)} {}

EnumExpression::~EnumExpression() = default;

auto EnumExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto EnumExpression::parse(Parser& parser)
    -> Expected<std::unique_ptr<EnumExpression>, ParserDiagnostic> {
    TODO(parser);
}

} // namespace ast
