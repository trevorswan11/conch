#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "core.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace ast {

class BlockStatement : public Statement {
  public:
    explicit BlockStatement(const Token&                            start_token,
                            std::vector<std::unique_ptr<Statement>> statements) noexcept
        : Statement{start_token}, statements_{std::move(statements)} {}

    auto accept(Visitor& v) -> void override;

    static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<BlockStatement>, ParserDiagnostic>;

  private:
    std::vector<std::unique_ptr<Statement>> statements_;
};

} // namespace ast
