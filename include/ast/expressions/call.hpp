#pragma once

#include <memory>
#include <span>
#include <utility>
#include <vector>

#include "core.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace ast {

struct CallArgument {
    explicit CallArgument(bool r, std::unique_ptr<Expression> a) noexcept
        : reference{r}, argument{std::move(a)} {}

    bool                        reference;
    std::unique_ptr<Expression> argument;
};

class CallExpression : public Expression {
  public:
    explicit CallExpression(const Token&                               start_token,
                            std::unique_ptr<Expression>                function,
                            std::vector<std::unique_ptr<CallArgument>> arguments) noexcept
        : Expression{start_token}, function_{std::move(function)},
          arguments_{std::move(arguments)} {}

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<CallExpression>, ParserDiagnostic>;

    [[nodiscard]] auto function() const noexcept -> const Expression& { return *function_; }
    [[nodiscard]] auto arguments() const noexcept
        -> std::span<const std::unique_ptr<CallArgument>> {
        return arguments_;
    }

  private:
    std::unique_ptr<Expression>                function_;
    std::vector<std::unique_ptr<CallArgument>> arguments_;
};

} // namespace ast
