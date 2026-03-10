#pragma once

#include <span>

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class IdentifierExpression;

class Enumeration {
  public:
    explicit Enumeration(Box<IdentifierExpression> enumeration,
                         Optional<Box<Expression>> value) noexcept;
    ~Enumeration();

    MAKE_AST_COPY_MOVE(Enumeration)

    [[nodiscard]] auto get_name() const noexcept -> const IdentifierExpression& { return *name_; }

    MAKE_OPTIONAL_UNPACKER(default_value, Expression, value_, **)

  private:
    Box<IdentifierExpression> name_;
    Optional<Box<Expression>> value_;

    friend class EnumExpression;
};

class EnumExpression : public ExprBase<EnumExpression> {
  public:
    static constexpr auto KIND = NodeKind::ENUM_EXPRESSION;

  public:
    explicit EnumExpression(const Token&                        start_token,
                            Optional<Box<IdentifierExpression>> underlying,
                            std::vector<Enumeration>            enumerations) noexcept;
    ~EnumExpression() override;

    MAKE_AST_COPY_MOVE(EnumExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;

    MAKE_OPTIONAL_UNPACKER(underlying, IdentifierExpression, underlying_, **)

    [[nodiscard]] auto get_enumerations() const noexcept -> std::span<const Enumeration> {
        return enumerations_;
    }

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    Optional<Box<IdentifierExpression>> underlying_;
    std::vector<Enumeration>            enumerations_;
};

} // namespace conch::ast
