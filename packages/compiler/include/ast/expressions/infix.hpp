#pragma once

#include <utility>

#include "ast/node.hpp"
#include "ast/visitor.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

template <typename Derived> class InfixExpression : public ExprBase<Derived> {
  public:
    explicit InfixExpression(const Token&    start_token,
                             Box<Expression> lhs,
                             TokenType       op,
                             Box<Expression> rhs) noexcept
        : ExprBase<Derived>{start_token}, lhs_{std::move(lhs)}, op_{op}, rhs_{std::move(rhs)} {}

    MAKE_AST_GETTER(lhs, const Expression&, *)
    MAKE_AST_GETTER(op, TokenType, )
    MAKE_AST_GETTER(rhs, const Expression&, *)

    auto accept(Visitor& v) const noexcept -> void override { v.visit(Node::as<Derived>(*this)); }

    [[nodiscard]] static auto parse(Parser& parser, Box<Expression> lhs)
        -> Expected<Box<Expression>, ParserDiagnostic> {
        const auto op_token           = parser.current_token();
        const auto current_precedence = parser.poll_current_precedence();
        if (parser.peek_token_is(TokenType::END)) {
            return make_parser_unexpected(ParserError::INFIX_MISSING_RHS, op_token);
        }

        parser.advance();
        auto rhs = TRY(parser.parse_expression(current_precedence));
        return make_box<Derived>(lhs->get_token(), std::move(lhs), op_token.type, std::move(rhs));
    }

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override {
        const auto& casted = Node::as<Derived>(other);
        return *lhs_ == *casted.lhs_ && op_ == casted.op_ && *rhs_ == *casted.rhs_;
    }

  protected:
    Box<Expression> lhs_;
    TokenType       op_;
    Box<Expression> rhs_;
};

#define DECLARE_INFIX_EXPRESSION(Type, Kind)    \
    class Type : public InfixExpression<Type> { \
      public:                                   \
        static constexpr auto KIND = Kind;      \
                                                \
      public:                                   \
        using InfixExpression::InfixExpression; \
        MAKE_AST_COPY_MOVE(Type)                \
                                                \
        using InfixExpression::parse;           \
    };

DECLARE_INFIX_EXPRESSION(AssignmentExpression, NodeKind::ASSIGNMENT_EXPRESSION)
DECLARE_INFIX_EXPRESSION(BinaryExpression, NodeKind::BINARY_EXPRESSION)
DECLARE_INFIX_EXPRESSION(DotExpression, NodeKind::DOT_EXPRESSION)
DECLARE_INFIX_EXPRESSION(RangeExpression, NodeKind::RANGE_EXPRESSION)
DECLARE_INFIX_EXPRESSION(ImplicitDereferenceExpression, NodeKind::IMPLICIT_DEREFERENCE_EXPRESSION)

#undef DECLARE_INFIX_EXPRESSION

} // namespace conch::ast
