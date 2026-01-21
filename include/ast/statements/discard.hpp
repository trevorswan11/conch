#pragma once

#include <memory>
#include <utility>

#include "core.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace ast {

class DiscardStatement : public Statement {
  public:
    explicit DiscardStatement(const Token&                start_token,
                              std::unique_ptr<Expression> to_discard) noexcept
        : Statement{start_token}, to_discard_{std::move(to_discard)} {}

    auto accept(Visitor& v) -> void override;

    static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<DiscardStatement>, ParserDiagnostic>;

  private:
    std::unique_ptr<Expression> to_discard_;
};

} // namespace ast
