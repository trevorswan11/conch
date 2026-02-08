#pragma once

#include <utility>

#include "util/common.hpp"
#include "util/expected.hpp"
#include "util/optional.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class JumpStatement : public KindStatement<JumpStatement> {
  public:
    static constexpr auto KIND = NodeKind::JUMP_STATEMENT;

  public:
    explicit JumpStatement(const Token& start_token, Optional<Box<Expression>> expression) noexcept
        : KindStatement{start_token}, expression_{std::move(expression)} {}

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Statement>, ParserDiagnostic>;

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
