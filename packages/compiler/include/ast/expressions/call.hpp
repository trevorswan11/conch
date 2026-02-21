#pragma once

#include <algorithm>
#include <span>
#include <utility>
#include <vector>

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class CallExpression : public ExprBase<CallExpression> {
  public:
    static constexpr auto KIND = NodeKind::CALL_EXPRESSION;

  public:
    explicit CallExpression(const Token&                 start_token,
                            Box<Expression>              function,
                            std::vector<Box<Expression>> arguments) noexcept
        : ExprBase{start_token}, function_{std::move(function)}, arguments_{std::move(arguments)} {}

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser, Box<Expression> function)
        -> Expected<Box<Expression>, ParserDiagnostic>;

    [[nodiscard]] auto get_function() const noexcept -> const Expression& { return *function_; }
    [[nodiscard]] auto get_arguments() const noexcept -> std::span<const Box<Expression>> {
        return arguments_;
    }

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override {
        const auto& casted = as<CallExpression>(other);
        return *function_ == *casted.function_ && std::ranges::equal(arguments_, casted.arguments_);
    }

  private:
    Box<Expression>              function_;
    std::vector<Box<Expression>> arguments_;
};

} // namespace conch::ast
