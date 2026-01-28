#pragma once

#include <memory>
#include <span>

#include "util/expected.hpp"
#include "util/optional.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace ast {

class IdentifierExpression;

class Enumeration {
  public:
    explicit Enumeration(std::unique_ptr<IdentifierExpression> enumeration) noexcept;
    explicit Enumeration(std::unique_ptr<IdentifierExpression> enumeration,
                         Optional<std::unique_ptr<Expression>> value) noexcept;
    ~Enumeration();

    Enumeration(const Enumeration&)                        = delete;
    auto operator=(const Enumeration&) -> Enumeration&     = delete;
    Enumeration(Enumeration&&) noexcept                    = default;
    auto operator=(Enumeration&&) noexcept -> Enumeration& = default;

    [[nodiscard]] auto reference() const noexcept -> const IdentifierExpression& {
        return *enumeration_;
    }

    [[nodiscard]] auto argument() const noexcept -> Optional<const Expression&> {
        return value_ ? Optional<const Expression&>{**value_} : nullopt;
    }

  private:
    std::unique_ptr<IdentifierExpression> enumeration_;
    Optional<std::unique_ptr<Expression>> value_;
};

class EnumExpression : public Expression {
  public:
    explicit EnumExpression(const Token&                          start_token,
                            std::unique_ptr<IdentifierExpression> name,
                            std::vector<Enumeration>              variants) noexcept;
    ~EnumExpression() override;

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<EnumExpression>, ParserDiagnostic>;

    [[nodiscard]] auto name() const noexcept -> const IdentifierExpression& { return *name_; }
    [[nodiscard]] auto enumerations() const noexcept -> std::span<const Enumeration> {
        return enumerations_;
    }

  private:
    std::unique_ptr<IdentifierExpression> name_;
    std::vector<Enumeration>              enumerations_;
};

} // namespace ast
