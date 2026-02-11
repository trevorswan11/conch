#pragma once

#include <utility>

#include "util/common.hpp"
#include "util/expected.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class ArrayExpression : public ExprBase<ArrayExpression> {
  public:
    static constexpr auto KIND = NodeKind::ARRAY_EXPRESSION;

  public:
    explicit ArrayExpression(const Token&    start_token,
                             bool            inferred_size,
                             Box<Expression> items) noexcept
        : ExprBase{start_token}, inferred_size_{inferred_size}, items_{std::move(items)} {}

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;

    auto is_inferred() const noexcept -> bool { return inferred_size_; }
    auto get_items() const noexcept -> const Expression& { return *items_; }

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override {
        const auto& casted = as<ArrayExpression>(other);
        return inferred_size_ == casted.inferred_size_ && *items_ == *casted.items_;
    }

  private:
    bool            inferred_size_;
    Box<Expression> items_;
};

} // namespace conch::ast
