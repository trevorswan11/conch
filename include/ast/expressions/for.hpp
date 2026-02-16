#pragma once

#include <span>
#include <vector>

#include "util/common.hpp"
#include "util/expected.hpp"
#include "util/optional.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class IdentifierExpression;
class BlockStatement;

class ForLoopCapture {
  public:
    explicit ForLoopCapture(bool reference, Box<IdentifierExpression> capture) noexcept;
    ~ForLoopCapture();

    ForLoopCapture(const ForLoopCapture&)                        = delete;
    auto operator=(const ForLoopCapture&) -> ForLoopCapture&     = delete;
    ForLoopCapture(ForLoopCapture&&) noexcept                    = default;
    auto operator=(ForLoopCapture&&) noexcept -> ForLoopCapture& = default;

    [[nodiscard]] auto is_reference() const noexcept -> bool { return reference_; }
    [[nodiscard]] auto get_capture() const noexcept -> const IdentifierExpression& {
        return *capture_;
    }

  private:
    bool                      reference_;
    Box<IdentifierExpression> capture_;

    friend class ForLoopExpression;
};

class ForLoopExpression : public ExprBase<ForLoopExpression> {
  public:
    static constexpr auto KIND = NodeKind::FOR_LOOP_EXPRESSION;

  public:
    explicit ForLoopExpression(const Token&                          start_token,
                               std::vector<Box<Expression>>          iterables,
                               Optional<std::vector<ForLoopCapture>> captures,
                               Box<BlockStatement>                   block,
                               Optional<Box<Statement>>              non_break) noexcept;
    ~ForLoopExpression() override;

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;

    [[nodiscard]] auto get_iterables() const noexcept -> std::span<const Box<Expression>> {
        return iterables_;
    }

    [[nodiscard]] auto get_captures() const noexcept -> Optional<std::span<const ForLoopCapture>> {
        return captures_;
    }

    [[nodiscard]] auto get_block() const noexcept -> const BlockStatement& { return *block_; }
    [[nodiscard]] auto has_non_break() const noexcept -> bool { return non_break_.has_value(); }
    [[nodiscard]] auto get_non_break() const noexcept -> Optional<const Statement&> {
        return non_break_ ? Optional<const Statement&>{**non_break_} : nullopt;
    }

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    std::vector<Box<Expression>>          iterables_;
    Optional<std::vector<ForLoopCapture>> captures_;
    Box<BlockStatement>                   block_;
    Optional<Box<Statement>>              non_break_;
};

} // namespace conch::ast
