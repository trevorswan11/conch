#include <utility>

#include "ast/statements/decl.hpp"

#include "ast/expressions/function.hpp"
#include "ast/expressions/identifier.hpp"
#include "ast/expressions/type.hpp"

#include "visitor/visitor.hpp"

namespace ast {

DeclStatement::DeclStatement(const Token&                          start_token,
                             std::unique_ptr<IdentifierExpression> ident,
                             std::unique_ptr<TypeExpression>       type,
                             Optional<std::unique_ptr<Expression>> value,
                             DeclarationMetadata                   metadata) noexcept
    : Statement{start_token}, ident_{std::move(ident)}, type_{std::move(type)},
      value_{std::move(value)}, metadata_{std::move(metadata)} {}

DeclStatement::~DeclStatement() = default;

auto DeclStatement::accept(Visitor& v) const -> void { v.visit(*this); }

auto DeclStatement::parse(Parser& parser)
    -> Expected<std::unique_ptr<DeclStatement>, ParserDiagnostic> {
    TODO(parser);
}

} // namespace ast
