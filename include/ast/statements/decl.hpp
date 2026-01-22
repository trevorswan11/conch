#pragma once

#include <memory>
#include <optional>

#include "core.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace ast {

class IdentifierExpression;
class TypeExpression;

enum class DeclarationMetadata {
    MUTABLE_VALUE,
    CONSTANT_VALUE,
    TYPE_ALIAS,
};

class DeclStatement : public Statement {
  public:
    explicit DeclStatement(const Token&                               start_token,
                           std::unique_ptr<IdentifierExpression>      ident,
                           std::unique_ptr<TypeExpression>            type,
                           std::optional<std::unique_ptr<Expression>> value,
                           DeclarationMetadata                        metadata) noexcept;
    ~DeclStatement();

    auto accept(Visitor& v) const -> void override;

    static auto parse(Parser& parser) -> Expected<std::unique_ptr<DeclStatement>, ParserDiagnostic>;

  private:
    std::unique_ptr<IdentifierExpression>      ident_;
    std::unique_ptr<TypeExpression>            type_;
    std::optional<std::unique_ptr<Expression>> value_;
    DeclarationMetadata                        metadata_;
};

} // namespace ast
