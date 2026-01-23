#pragma once

#include <memory>
#include <optional>
#include <span>
#include <vector>

#include "core.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace ast {

class BlockStatement;

struct ForLoopCapture {
    explicit ForLoopCapture(bool r, std::unique_ptr<Expression> c) noexcept
        : reference{r}, capture{std::move(c)} {}

    bool                        reference;
    std::unique_ptr<Expression> capture;
};

class ForLoopExpression : public Expression {
  public:
    explicit ForLoopExpression(const Token&                               start_token,
                               std::vector<std::unique_ptr<Expression>>   iterables,
                               std::vector<std::optional<ForLoopCapture>> captures,
                               std::unique_ptr<BlockStatement>            block,
                               std::unique_ptr<Statement>                 non_break) noexcept;
    ~ForLoopExpression() override;

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<ForLoopExpression>, ParserDiagnostic>;

    [[nodiscard]] auto iterables() const noexcept -> std::span<const std::unique_ptr<Expression>> {
        return iterables_;
    }
    [[nodiscard]] auto captures() const noexcept -> std::span<const std::optional<ForLoopCapture>> {
        return captures_;
    }
    [[nodiscard]] auto block() const noexcept -> const BlockStatement& { return *block_; }
    [[nodiscard]] auto non_break() const noexcept -> std::optional<const Statement*> {
        if (non_break_) { return non_break_->get(); }
        return std::nullopt;
    }

  private:
    std::vector<std::unique_ptr<Expression>>   iterables_;
    std::vector<std::optional<ForLoopCapture>> captures_;
    std::unique_ptr<BlockStatement>            block_;
    std::optional<std::unique_ptr<Statement>>  non_break_;
};

} // namespace ast
