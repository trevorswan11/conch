#pragma once

#include <span>
#include <vector>

#include "ast/expressions/type.hpp"
#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class IdentifierExpression;

class UnionField {
  public:
    explicit UnionField(Box<IdentifierExpression> ident, ExplicitType&& type) noexcept;
    ~UnionField();

    MAKE_AST_COPY_MOVE(UnionField)

    MAKE_AST_GETTER(ident, const IdentifierExpression&, *)
    MAKE_AST_GETTER(type, const ExplicitType&, )

    MAKE_AST_DEPENDENT_EQ(UnionField)

  private:
    Box<IdentifierExpression> ident_;
    ExplicitType              type_;
};

class UnionExpression : public ExprBase<UnionExpression> {
  public:
    static constexpr auto KIND = NodeKind::UNION_EXPRESSION;

  public:
    explicit UnionExpression(const Token& start_token, std::vector<UnionField> fields) noexcept;
    ~UnionExpression() override;

    MAKE_AST_COPY_MOVE(UnionExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;

    MAKE_AST_GETTER(fields, std::span<const UnionField>, )

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    std::vector<UnionField> fields_;
};

} // namespace conch::ast
