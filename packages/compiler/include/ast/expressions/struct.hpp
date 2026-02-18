#pragma once

#include <span>

#include "expected.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class FunctionExpression;

class DeclStatement;

class StructExpression : public ExprBase<StructExpression> {
  public:
    static constexpr auto KIND = NodeKind::STRUCT_EXPRESSION;

  public:
    explicit StructExpression(const Token&                    start_token,
                              std::vector<Box<DeclStatement>> members) noexcept;
    ~StructExpression() override;

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;

    [[nodiscard]] auto is_packed() const noexcept -> bool {
        return start_token_.type == TokenType::PACKED;
    }

    [[nodiscard]] auto get_members() const noexcept -> std::span<const Box<DeclStatement>> {
        return members_;
    }

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    std::vector<Box<DeclStatement>> members_;
};

} // namespace conch::ast
