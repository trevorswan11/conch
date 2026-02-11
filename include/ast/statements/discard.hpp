#pragma once

#include <utility>

#include "util/expected.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class DiscardStatement : public StmtBase<DiscardStatement> {
  public:
    static constexpr auto KIND = NodeKind::DISCARD_STATEMENT;

  public:
    explicit DiscardStatement(const Token& start_token, Box<Expression> discarded) noexcept
        : StmtBase{start_token}, discarded_{std::move(discarded)} {}

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Statement>, ParserDiagnostic>;

    [[nodiscard]] auto get_discarded() const noexcept -> const Expression& { return *discarded_; }

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override {
        const auto& casted = as<DiscardStatement>(other);
        return *discarded_ == *casted.discarded_;
    }

  private:
    Box<Expression> discarded_;
};

} // namespace conch::ast
