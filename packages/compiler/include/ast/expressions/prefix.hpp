#pragma once

#include <utility>

#include "ast/node.hpp"
#include "ast/visitor.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

template <typename Derived> class PrefixExpression : public ExprBase<Derived> {
  public:
    explicit PrefixExpression(const Token& start_token, Box<Expression> rhs) noexcept
        : ExprBase<Derived>{start_token}, rhs_{std::move(rhs)} {}

    auto accept(Visitor& v) const noexcept -> void override { v.visit(Node::as<Derived>(*this)); }

    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic> {
        const auto prefix_token = parser.current_token();
        if (parser.peek_token_is(TokenType::END)) {
            return make_parser_unexpected(ParserError::PREFIX_MISSING_OPERAND, prefix_token);
        }
        parser.advance();

        auto operand = TRY(parser.parse_expression(Precedence::PREFIX));
        return make_box<Derived>(prefix_token, std::move(operand));
    }

    [[nodiscard]] auto get_op() const noexcept -> TokenType { return this->start_token_.type; }
    MAKE_AST_GETTER(rhs, const Expression&, *)

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override {
        const auto& casted = Node::as<Derived>(other);
        return *rhs_ == *casted.rhs_;
    }

  private:
    Box<Expression> rhs_;
};

#define DECLARE_PREFIX_EXPRESSION(Type, Kind)     \
    class Type : public PrefixExpression<Type> {  \
      public:                                     \
        static constexpr auto KIND = Kind;        \
                                                  \
      public:                                     \
        using PrefixExpression::PrefixExpression; \
        MAKE_AST_COPY_MOVE(Type)                  \
                                                  \
        using PrefixExpression::parse;            \
    };

DECLARE_PREFIX_EXPRESSION(UnaryExpression, NodeKind::UNARY_EXPRESSION)
DECLARE_PREFIX_EXPRESSION(ReferenceExpression, NodeKind::REFERENCE_EXPRESSION)
DECLARE_PREFIX_EXPRESSION(DereferenceExpression, NodeKind::DEREFERENCE_EXPRESSION)
DECLARE_PREFIX_EXPRESSION(ImplicitAccessExpression, NodeKind::IMPLICIT_ACCESS_EXPRESSION)

#undef DECLARE_PREFIX_EXPRESSION

} // namespace conch::ast
