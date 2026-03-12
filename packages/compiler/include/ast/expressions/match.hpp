#pragma once

#include <algorithm>
#include <span>
#include <utility>
#include <variant>

#include "ast/node.hpp"

#include "parser/parser.hpp"

#include "variant.hpp"

namespace conch::ast {

class IdentifierExpression;

class MatchArm {
  public:
    using Capture = std::variant<Box<IdentifierExpression>, std::monostate>;

  public:
    explicit MatchArm(Box<Expression>   pattern,
                      Optional<Capture> capture,
                      Box<Statement>    dispatch) noexcept;
    ~MatchArm();

    MAKE_AST_COPY_MOVE(MatchArm)

    MAKE_AST_GETTER(pattern, const Expression&, *)
    [[nodiscard]] auto has_capture_clause() const noexcept -> bool { return capture_.has_value(); }
    MAKE_VARIANT_UNPACKER(
        explicit_capture, IdentifierExpression, Box<IdentifierExpression>, *capture_, *std::get)

    [[nodiscard]] auto is_discarded_capture() const noexcept -> bool {
        return std::holds_alternative<std::monostate>(*capture_);
    }

    MAKE_AST_GETTER(dispatch, const Statement&, *)

    MAKE_AST_DEPENDENT_EQ(MatchArm)

  private:
    Box<Expression>   pattern_;
    Optional<Capture> capture_;
    Box<Statement>    dispatch_;
};

class MatchExpression : public ExprBase<MatchExpression> {
  public:
    static constexpr auto KIND = NodeKind::MATCH_EXPRESSION;

  public:
    explicit MatchExpression(const Token&             start_token,
                             Box<Expression>          matcher,
                             std::vector<MatchArm>    arms,
                             Optional<Box<Statement>> catch_all) noexcept
        : ExprBase{start_token}, matcher_{std::move(matcher)}, arms_{std::move(arms)},
          catch_all_{std::move(catch_all)} {}

    MAKE_AST_COPY_MOVE(MatchExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;

    MAKE_AST_GETTER(matcher, const Expression&, *)
    MAKE_AST_GETTER(arms, std::span<const MatchArm>, )
    MAKE_OPTIONAL_UNPACKER(catch_all, Statement, catch_all_, **)

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override {
        const auto& casted  = as<MatchExpression>(other);
        const auto  arms_eq = std::ranges::equal(arms_, casted.arms_);
        return *matcher_ == *casted.matcher_ && arms_eq &&
               optional::unsafe_eq<Statement>(catch_all_, casted.catch_all_);
    }

  private:
    Box<Expression>          matcher_;
    std::vector<MatchArm>    arms_;
    Optional<Box<Statement>> catch_all_;
};

} // namespace conch::ast
