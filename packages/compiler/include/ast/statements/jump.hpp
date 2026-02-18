#pragma once

#include <utility>

#include "expected.hpp"
#include "optional.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class JumpStatement : public StmtBase<JumpStatement> {
  public:
    static constexpr auto KIND = NodeKind::JUMP_STATEMENT;

  public:
    explicit JumpStatement(const Token& start_token, Optional<Box<Expression>> expression) noexcept
        : StmtBase{start_token}, expression_{std::move(expression)} {}

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Statement>, ParserDiagnostic>;

    [[nodiscard]] auto has_expression() const noexcept -> bool { return expression_.has_value(); }
    [[nodiscard]] auto get_expression() const noexcept -> Optional<const Expression&> {
        return expression_ ? Optional<const Expression&>{**expression_} : nullopt;
    }

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override {
        const auto& casted = as<JumpStatement>(other);
        return optional::unsafe_eq<Expression>(expression_, casted.expression_);
    }

  private:
    Optional<Box<Expression>> expression_;
};

} // namespace conch::ast
