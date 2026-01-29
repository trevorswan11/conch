#pragma once

#include <utility>

#include "util/expected.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class DiscardStatement : public Statement {
  public:
    explicit DiscardStatement(const Token& start_token, Box<Expression> discarded) noexcept
        : Statement{start_token}, discarded_{std::move(discarded)} {}

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<Box<DiscardStatement>, ParserDiagnostic>;

    [[nodiscard]] auto get_discarded() const noexcept -> const Expression& { return *discarded_; }

  private:
    Box<Expression> discarded_;
};

} // namespace conch::ast
