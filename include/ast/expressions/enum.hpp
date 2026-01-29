#pragma once

#include <span>

#include "util/common.hpp"
#include "util/expected.hpp"
#include "util/optional.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class IdentifierExpression;

class Enumeration {
  public:
    explicit Enumeration(Box<IdentifierExpression> enumeration,
                         Optional<Box<Expression>> value) noexcept;
    ~Enumeration();

    Enumeration(const Enumeration&)                        = delete;
    auto operator=(const Enumeration&) -> Enumeration&     = delete;
    Enumeration(Enumeration&&) noexcept                    = default;
    auto operator=(Enumeration&&) noexcept -> Enumeration& = default;

    [[nodiscard]] auto get_enumeration() const noexcept -> const IdentifierExpression& {
        return *enumeration_;
    }

    [[nodiscard]] auto has_default_value() const noexcept -> bool { return value_.has_value(); }
    [[nodiscard]] auto get_default_value() const noexcept -> Optional<const Expression&> {
        return value_ ? Optional<const Expression&>{**value_} : nullopt;
    }

  private:
    Box<IdentifierExpression> enumeration_;
    Optional<Box<Expression>> value_;
};

class EnumExpression : public Expression {
  public:
    explicit EnumExpression(const Token&                        start_token,
                            Optional<Box<IdentifierExpression>> underlying,
                            std::vector<Enumeration>            enumerations) noexcept;
    ~EnumExpression() override;

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<Box<EnumExpression>, ParserDiagnostic>;

    [[nodiscard]] auto get_underlying() const noexcept -> Optional<const IdentifierExpression&> {
        return underlying_ ? Optional<const IdentifierExpression&>{**underlying_} : nullopt;
    }

    [[nodiscard]] auto get_enumerations() const noexcept -> std::span<const Enumeration> {
        return enumerations_;
    }

  private:
    Optional<Box<IdentifierExpression>> underlying_;
    std::vector<Enumeration>            enumerations_;
};

} // namespace conch::ast
