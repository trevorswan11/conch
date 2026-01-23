#include "ast/statements/import.hpp"

#include "ast/expressions/identifier.hpp"
#include "ast/expressions/primitive.hpp"

#include "visitor/visitor.hpp"

namespace ast {

ImportStatement::ImportStatement(
    const Token& start_token,
    std::variant<std::unique_ptr<IdentifierExpression>, std::unique_ptr<StringExpression>> import,
    std::optional<std::unique_ptr<IdentifierExpression>> alias) noexcept
    : Statement{start_token}, import_{std::move(import)}, alias_{std::move(alias)} {}

ImportStatement::~ImportStatement() = default;

auto ImportStatement::accept(Visitor& v) const -> void { v.visit(*this); }

auto ImportStatement::parse(Parser& parser)
    -> Expected<std::unique_ptr<ImportStatement>, ParserDiagnostic> {
    TODO(parser);
}

} // namespace ast
