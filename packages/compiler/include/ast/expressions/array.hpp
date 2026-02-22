#pragma once

#include <span>
#include <vector>

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class ArrayExpression : public ExprBase<ArrayExpression> {
  public:
    static constexpr auto KIND = NodeKind::ARRAY_EXPRESSION;

  public:
    explicit ArrayExpression(const Token&                 start_token,
                             Optional<Box<Expression>>    size,
                             std::vector<Box<Expression>> items) noexcept
        : ExprBase{start_token}, size_{std::move(size)}, items_{std::move(items)} {}
    ~ArrayExpression() override;

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;

    // Returns the array's size only if it was initialized with an explicit size.
    auto get_size() const noexcept -> Optional<const Expression&> {
        return size_ ? Optional<const Expression&>{**size_} : nullopt;
    }

    auto is_inferred_size() const noexcept -> bool { return !size_.has_value(); }
    auto get_items() const noexcept -> std::span<const Box<Expression>> { return items_; }

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    Optional<Box<Expression>>    size_;
    std::vector<Box<Expression>> items_;
};

} // namespace conch::ast
