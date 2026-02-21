#pragma once

#include <string>
#include <string_view>

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class IdentifierExpression : public ExprBase<IdentifierExpression> {
  public:
    static constexpr auto KIND = NodeKind::IDENTIFIER_EXPRESSION;

  public:
    explicit IdentifierExpression(const Token& start_token) noexcept : ExprBase{start_token} {}

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;

    [[nodiscard]] auto get_name() const noexcept -> std::string_view { return get_token().slice; }
    [[nodiscard]] auto materialize() const -> std::string { return std::string{get_name()}; }

  protected:
    auto is_equal(const Node&) const noexcept -> bool override { return true; }
};

} // namespace conch::ast
