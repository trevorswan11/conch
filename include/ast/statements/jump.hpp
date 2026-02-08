#pragma once

#include <utility>

#include "util/common.hpp"
#include "util/expected.hpp"
#include "util/optional.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class JumpStatement : public Statement {
  public:
    explicit JumpStatement(const Token& start_token, Optional<Box<Expression>> expression) noexcept
        : Statement{start_token, NodeKind::JUMP_STATEMENT}, expression_{std::move(expression)} {}

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<Box<JumpStatement>, ParserDiagnostic>;

    [[nodiscard]] auto has_expression() const noexcept -> bool { return expression_.has_value(); }
    [[nodiscard]] auto get_expression() const noexcept -> Optional<const Expression&> {
        return expression_ ? Optional<const Expression&>{**expression_} : nullopt;
    }

    auto is_equal(const Node& other) const noexcept -> bool override {
        const auto& casted = as<JumpStatement>(other);
        return optional::unsafe_eq<Expression>(expression_, casted.expression_);
    }

  private:
    Optional<Box<Expression>> expression_;
};

} // namespace conch::ast
