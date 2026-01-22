#pragma once

#include <memory>
#include <utility>

#include "core.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace ast {

class ArrayExpression : public Expression {
  public:
    explicit ArrayExpression(const Token&                start_token,
                             bool                        inferred_size,
                             std::unique_ptr<Expression> items) noexcept
        : Expression{start_token}, inferred_size_{inferred_size}, items_{std::move(items)} {}

    auto accept(Visitor& v) const -> void override;

    static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<ArrayExpression>, ParserDiagnostic>;

  private:
    bool                        inferred_size_;
    std::unique_ptr<Expression> items_;
};

} // namespace ast
