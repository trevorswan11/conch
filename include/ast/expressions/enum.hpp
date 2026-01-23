#pragma once

#include <memory>
#include <span>

#include "core.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace ast {

class IdentifierExpression;

struct EnumVariant {
    explicit EnumVariant(std::unique_ptr<IdentifierExpression> e) noexcept;
    explicit EnumVariant(std::unique_ptr<IdentifierExpression> e,
                         Optional<std::unique_ptr<Expression>> v) noexcept;
    ~EnumVariant();

    EnumVariant(const EnumVariant&)                        = delete;
    auto operator=(const EnumVariant&) -> EnumVariant&     = delete;
    EnumVariant(EnumVariant&&) noexcept                    = default;
    auto operator=(EnumVariant&&) noexcept -> EnumVariant& = default;

    std::unique_ptr<IdentifierExpression> enumeration;
    Optional<std::unique_ptr<Expression>> value;
};

class EnumExpression : public Expression {
  public:
    explicit EnumExpression(const Token&                          start_token,
                            std::unique_ptr<IdentifierExpression> name,
                            std::vector<EnumVariant>              variants) noexcept;
    ~EnumExpression() override;

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<EnumExpression>, ParserDiagnostic>;

    [[nodiscard]] auto name() const noexcept -> const IdentifierExpression& { return *name_; }
    [[nodiscard]] auto variants() const noexcept -> std::span<const EnumVariant> {
        return variants_;
    }

  private:
    std::unique_ptr<IdentifierExpression> name_;
    std::vector<EnumVariant>              variants_;
};

} // namespace ast
