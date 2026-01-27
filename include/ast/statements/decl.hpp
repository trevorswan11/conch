#pragma once

#include <memory>

#include "core.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace ast {

class IdentifierExpression;
class TypeExpression;

class DeclStatement : public Statement {
  public:
    explicit DeclStatement(const Token&                          start_token,
                           std::unique_ptr<IdentifierExpression> ident,
                           std::unique_ptr<TypeExpression>       type,
                           Optional<std::unique_ptr<Expression>> value,
                           bool                                  constant) noexcept;
    ~DeclStatement() override;

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<DeclStatement>, ParserDiagnostic>;

    [[nodiscard]] auto ident() const noexcept -> const IdentifierExpression& { return *ident_; }
    [[nodiscard]] auto type() const noexcept -> const TypeExpression& { return *type_; }
    [[nodiscard]] auto value() const noexcept -> Optional<const Expression&> {
        return value_ ? Optional<const Expression&>{**value_} : nullopt;
    }
    [[nodiscard]] auto constant() const noexcept -> bool { return constant_; }

  private:
    std::unique_ptr<IdentifierExpression> ident_;
    std::unique_ptr<TypeExpression>       type_;
    Optional<std::unique_ptr<Expression>> value_;
    bool                                  constant_;
};

} // namespace ast
