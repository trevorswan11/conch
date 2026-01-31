#pragma once

#include <span>
#include <utility>

#include "util/common.hpp"
#include "util/expected.hpp"
#include "util/optional.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class MatchArm {
  public:
    explicit MatchArm(Box<Expression> pattern, Box<Statement> dispatch) noexcept
        : pattern_{std::move(pattern)}, dispatch_{std::move(dispatch)} {}

    [[nodiscard]] auto get_pattern() const noexcept -> const Expression& { return *pattern_; }
    [[nodiscard]] auto get_dispatch() const noexcept -> const Statement& { return *dispatch_; }

  private:
    Box<Expression> pattern_;
    Box<Statement>  dispatch_;
};

class MatchExpression : public Expression {
  public:
    explicit MatchExpression(const Token&             start_token,
                             Box<Expression>          matcher,
                             std::vector<MatchArm>    arms,
                             Optional<Box<Statement>> catch_all) noexcept
        : Expression{start_token, NodeKind::MATCH_EXPRESSION}, matcher_{std::move(matcher)},
          arms_{std::move(arms)}, catch_all_{std::move(catch_all)} {};

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<Box<MatchExpression>, ParserDiagnostic>;

    [[nodiscard]] auto get_matcher() const noexcept -> const Expression& { return *matcher_; }
    [[nodiscard]] auto get_arms() const noexcept -> std::span<const MatchArm> { return arms_; }
    [[nodiscard]] auto has_catch_all() const noexcept -> bool { return catch_all_.has_value(); }
    [[nodiscard]] auto get_catch_all() const noexcept -> Optional<const Statement&> {
        return catch_all_ ? Optional<const Statement&>{**catch_all_} : nullopt;
    }

  private:
    Box<Expression>          matcher_;
    std::vector<MatchArm>    arms_;
    Optional<Box<Statement>> catch_all_;
};

} // namespace conch::ast
