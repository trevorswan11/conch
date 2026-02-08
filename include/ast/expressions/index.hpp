#pragma once

#include <utility>

#include "util/common.hpp"
#include "util/expected.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class IndexExpression : public Expression {
  public:
    explicit IndexExpression(const Token&    start_token,
                             Box<Expression> array,
                             Box<Expression> idx) noexcept
        : Expression{start_token, NodeKind::INDEX_EXPRESSION}, array_{std::move(array)},
          index_{std::move(idx)} {}

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser, Box<Expression> array)
        -> Expected<Box<IndexExpression>, ParserDiagnostic>;

    [[nodiscard]] auto get_array() const noexcept -> const Expression& { return *array_; }
    [[nodiscard]] auto get_index() const noexcept -> const Expression& { return *index_; }

    auto is_equal(const Node& other) const noexcept -> bool override {
        const auto& casted = as<IndexExpression>(other);
        return *array_ == *casted.array_ && *index_ == *casted.index_;
    }

  private:
    Box<Expression> array_;
    Box<Expression> index_;
};

} // namespace conch::ast
