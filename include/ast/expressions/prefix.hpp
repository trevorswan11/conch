#pragma once

#include <utility>

#include "util/common.hpp"
#include "util/expected.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class PrefixExpression : public KindExpression<PrefixExpression> {
  public:
    static constexpr auto KIND = NodeKind::PREFIX_EXPRESSION;

  public:
    explicit PrefixExpression(const Token& op, Box<Expression> rhs) noexcept
        : KindExpression{op}, rhs_{std::move(rhs)} {}

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;

    auto               get_op() const noexcept -> TokenType { return start_token_.type; }
    [[nodiscard]] auto get_rhs() const noexcept -> const Expression& { return *rhs_; }

    auto is_equal(const Node& other) const noexcept -> bool override {
        const auto& casted = as<PrefixExpression>(other);
        return *rhs_ == *casted.rhs_;
    }

  private:
    Box<Expression> rhs_;
};

} // namespace conch::ast
