#pragma once

#include <memory>
#include <utility>

#include "util/expected.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace ast {

class IndexExpression : public Expression {
  public:
    explicit IndexExpression(const Token&                start_token,
                             std::unique_ptr<Expression> array,
                             std::unique_ptr<Expression> idx) noexcept
        : Expression{start_token}, array_{std::move(array)}, idx_{std::move(idx)} {}

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser, std::unique_ptr<Expression> array)
        -> Expected<std::unique_ptr<IndexExpression>, ParserDiagnostic>;

    [[nodiscard]] auto array() const noexcept -> const Expression& { return *array_; }
    [[nodiscard]] auto idx() const noexcept -> const Expression& { return *idx_; }

  private:
    std::unique_ptr<Expression> array_;
    std::unique_ptr<Expression> idx_;
};

} // namespace ast
