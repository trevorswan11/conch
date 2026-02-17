#pragma once

#include <utility>

#include "core/common.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

#include "ast/visitor.hpp"

namespace conch::ast {

template <typename Derived> class InfixExpression : public ExprBase<Derived> {
  public:
    explicit InfixExpression(const Token&    start_token,
                             Box<Expression> lhs,
                             TokenType       op,
                             Box<Expression> rhs) noexcept
        : ExprBase<Derived>{start_token}, lhs_{std::move(lhs)}, op_{op}, rhs_{std::move(rhs)} {}

    [[nodiscard]] auto get_lhs() const noexcept -> const Expression& { return *lhs_; }
    auto               get_op() const noexcept -> TokenType { return op_; }
    [[nodiscard]] auto get_rhs() const noexcept -> const Expression& { return *rhs_; }

    auto accept(Visitor& v) const noexcept -> void override { v.visit(Node::as<Derived>(*this)); }

    [[nodiscard]] static auto parse(Parser& parser, Box<Expression> lhs)
        -> Expected<Box<Expression>, ParserDiagnostic> {
        const auto op_token_type      = parser.current_token().type;
        const auto current_precedence = parser.poll_current_precedence();

        parser.advance();
        auto rhs = TRY(parser.parse_expression(current_precedence));
        return make_box<Derived>(lhs->get_token(), std::move(lhs), op_token_type, std::move(rhs));
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

} // namespace conch::ast
