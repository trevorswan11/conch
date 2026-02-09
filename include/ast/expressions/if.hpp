#pragma once

#include <utility>

#include "util/common.hpp"
#include "util/expected.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class IfExpression : public ExprBase<IfExpression> {
  public:
    static constexpr auto KIND = NodeKind::IF_EXPRESSION;

  public:
    explicit IfExpression(const Token&             start_token,
                          Box<Expression>          condition,
                          Box<Statement>           consequence,
                          Optional<Box<Statement>> alternate) noexcept
        : ExprBase{start_token}, condition_{std::move(condition)},
          consequence_{std::move(consequence)}, alternate_{std::move(alternate)} {}

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;

    [[nodiscard]] auto get_condition() const noexcept -> const Expression& { return *condition_; }
    [[nodiscard]] auto get_consequence() const noexcept -> const Statement& {
        return *consequence_;
    }

    [[nodiscard]] auto has_alternate() const noexcept -> bool { return alternate_.has_value(); }
    [[nodiscard]] auto get_alternate() const noexcept -> Optional<const Statement&> {
        return alternate_ ? Optional<const Statement&>{**alternate_} : nullopt;
    }

    auto is_equal(const Node& other) const noexcept -> bool override {
        const auto& casted = as<IfExpression>(other);
        return *condition_ == *casted.condition_ && *consequence_ == *casted.consequence_ &&
               optional::unsafe_eq<Statement>(alternate_, casted.alternate_);
    }

  private:
    Box<Expression>          condition_;
    Box<Statement>           consequence_;
    Optional<Box<Statement>> alternate_;
};

} // namespace conch::ast
