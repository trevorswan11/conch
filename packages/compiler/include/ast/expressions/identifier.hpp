#pragma once

#include <string>
#include <string_view>

#include <fmt/format.h>

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class IdentifierExpression : public ExprBase<IdentifierExpression> {
  public:
    static constexpr auto KIND = NodeKind::IDENTIFIER_EXPRESSION;

  public:
    explicit IdentifierExpression(const Token& start_token) noexcept : ExprBase{start_token} {}

    MAKE_AST_COPY_MOVE(IdentifierExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;

    [[nodiscard]] auto get_name() const noexcept -> std::string_view { return get_token().slice; }
    [[nodiscard]] auto materialize() const -> std::string { return std::string{get_name()}; }

  protected:
    auto is_equal(const Node&) const noexcept -> bool override { return true; }
};

} // namespace conch::ast

template <> struct fmt::formatter<conch::ast::IdentifierExpression> {
    static constexpr auto parse(format_parse_context& ctx) noexcept { return ctx.begin(); }

    template <typename F> static auto format(const conch::ast::IdentifierExpression& n, F& ctx) {
        return fmt::format_to(ctx.out(), "{}", n.get_name());
    }
};
