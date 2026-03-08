#pragma once

#include <span>
#include <vector>

#include "ast/expressions/type_modifiers.hpp"
#include "ast/node.hpp"

#include "parser/parser.hpp"

#include "variant.hpp"

namespace conch::ast {

class BlockStatement;
class IdentifierExpression;

class ForLoopCapture {
  public:
    class Valued {
      public:
        explicit Valued(TypeModifier modifier, Box<IdentifierExpression> name) noexcept;
        ~Valued();

        MAKE_AST_COPY_MOVE(Valued)

        [[nodiscard]] auto get_modifier() const noexcept -> const TypeModifier& {
            return modifier_;
        }

        [[nodiscard]] auto get_name() const noexcept -> const IdentifierExpression& {
            return *name_;
        }

      private:
        TypeModifier              modifier_;
        Box<IdentifierExpression> name_;

        friend class ForLoopCapture;
    };

  public:
    explicit ForLoopCapture() noexcept;
    explicit ForLoopCapture(Valued valued) noexcept;
    ~ForLoopCapture();

    MAKE_AST_COPY_MOVE(ForLoopCapture)

    [[nodiscard]] auto is_discarded() const noexcept -> bool {
        return std::holds_alternative<std::monostate>(underlying_);
    }

    MAKE_VARIANT_UNPACKER(valued, Valued, Valued, underlying_, std::get)

    friend auto operator==(const ForLoopCapture& lhs, const ForLoopCapture& rhs) noexcept -> bool {
        return lhs.is_equal(rhs);
    }

  private:
    auto is_equal(const ForLoopCapture& other) const noexcept -> bool;

  private:
    std::variant<std::monostate, Valued> underlying_;

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

    MAKE_AST_COPY_MOVE(ForLoopExpression)

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
