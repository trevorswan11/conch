#include "ast/expressions/identifier.hpp"
#include "ast/expressions/primitive.hpp"

#include "ast/statements/import.hpp"

namespace ast {

ImportStatement::ImportStatement(
    const Token& start_token,
    std::variant<std::unique_ptr<IdentifierExpression>, std::unique_ptr<StringExpression>> import,
    std::optional<std::unique_ptr<IdentifierExpression>> alias) noexcept
    : Statement{start_token}, import_{std::move(import)}, alias_{std::move(alias)} {}

} // namespace ast
