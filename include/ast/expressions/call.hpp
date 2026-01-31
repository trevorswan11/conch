#pragma once

#include <algorithm>
#include <span>
#include <utility>
#include <vector>

#include "util/common.hpp"
#include "util/expected.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class CallArgument {
  public:
    explicit CallArgument(bool reference, Box<Expression> argument) noexcept
        : reference_{reference}, argument_{std::move(argument)} {}

    [[nodiscard]] auto is_reference() const noexcept -> bool { return reference_; }
    [[nodiscard]] auto get_argument() const noexcept -> const Expression& { return *argument_; }

  private:
    bool            reference_;
    Box<Expression> argument_;

    friend class CallExpression;
};

class CallExpression : public Expression {
  public:
    explicit CallExpression(const Token&              start_token,
                            Box<Expression>           function,
                            std::vector<CallArgument> arguments) noexcept
        : Expression{start_token, NodeKind::CALL_EXPRESSION}, function_{std::move(function)},
          arguments_{std::move(arguments)} {}

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser, Box<Expression> function)
        -> Expected<Box<CallExpression>, ParserDiagnostic>;

    [[nodiscard]] auto get_function() const noexcept -> const Expression& { return *function_; }
    [[nodiscard]] auto get_arguments() const noexcept -> std::span<const CallArgument> {
        return arguments_;
    }

    auto is_equal(const Node& other) const noexcept -> bool override {
        const auto& casted = as<CallExpression>(other);
        return *function_ == *casted.function_ &&
               std::ranges::equal(arguments_, casted.arguments_, [](const auto& a, const auto& b) {
                   return a.reference_ == b.reference_ && a.argument_ == b.argument_;
               });
    }

  private:
    Box<Expression>           function_;
    std::vector<CallArgument> arguments_;
};

} // namespace conch::ast
