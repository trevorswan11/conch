#pragma once

#include <memory>
#include <span>

#include "core.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace ast {

class IdentifierExpression;
class TypeExpression;

class StructMember {
  public:
    explicit StructMember(std::unique_ptr<IdentifierExpression> name,
                          std::unique_ptr<TypeExpression>       type) noexcept;
    explicit StructMember(std::unique_ptr<IdentifierExpression> name,
                          std::unique_ptr<TypeExpression>       type,
                          Optional<std::unique_ptr<Expression>> default_value) noexcept;
    ~StructMember();

    StructMember(const StructMember&)                        = delete;
    auto operator=(const StructMember&) -> StructMember&     = delete;
    StructMember(StructMember&&) noexcept                    = default;
    auto operator=(StructMember&&) noexcept -> StructMember& = default;

    [[nodiscard]] auto name() const noexcept -> const IdentifierExpression& { return *name_; }
    [[nodiscard]] auto type() const noexcept -> const TypeExpression& { return *type_; }
    [[nodiscard]] auto default_value() const noexcept -> Optional<const Expression&> {
        return default_value_ ? Optional<const Expression&>{**default_value_} : nullopt;
    }

  private:
    std::unique_ptr<IdentifierExpression> name_;
    std::unique_ptr<TypeExpression>       type_;
    Optional<std::unique_ptr<Expression>> default_value_;
};

class StructExpression : public Expression {
  public:
    explicit StructExpression(const Token&                          start_token,
                              std::unique_ptr<IdentifierExpression> name,
                              std::vector<StructMember>             members) noexcept;
    ~StructExpression() override;

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<StructExpression>, ParserDiagnostic>;

    [[nodiscard]] auto name() const noexcept -> const IdentifierExpression& { return *name_; }
    [[nodiscard]] auto members() const noexcept -> std::span<const StructMember> {
        return members_;
    }

  private:
    std::unique_ptr<IdentifierExpression> name_;
    std::vector<StructMember>             members_;
};

} // namespace ast
