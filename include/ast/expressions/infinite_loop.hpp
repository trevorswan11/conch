#pragma once

#include <memory>

#include "core.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace ast {

class BlockStatement;

class InfiniteLoopExpression : public Expression {
  public:
    explicit InfiniteLoopExpression(const Token&                    start_token,
                                    std::unique_ptr<BlockStatement> block) noexcept;
    ~InfiniteLoopExpression() override;

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<InfiniteLoopExpression>, ParserDiagnostic>;

    [[nodiscard]] auto block() const noexcept -> const BlockStatement& { return *block_; }

  private:
    std::unique_ptr<BlockStatement> block_;
};

} // namespace ast
