#pragma once

#include <memory>
#include <span>
#include <utility>

#include "core.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace ast {

class MatchArm {
  public:
    explicit MatchArm(std::unique_ptr<Expression> pattern,
                      std::unique_ptr<Statement>  dispatch) noexcept
        : pattern_{std::move(pattern)}, dispatch_{std::move(dispatch)} {}

    [[nodiscard]] auto pattern() const noexcept -> const Expression& { return *pattern_; }
    [[nodiscard]] auto dispatch() const noexcept -> const Statement& { return *dispatch_; }

  private:
    std::unique_ptr<Expression> pattern_;
    std::unique_ptr<Statement>  dispatch_;
};

class MatchExpression : public Expression {
  public:
    explicit MatchExpression(const Token&                         start_token,
                             std::unique_ptr<Expression>          matcher,
                             std::vector<MatchArm>                arms,
                             Optional<std::unique_ptr<Statement>> catch_all) noexcept
        : Expression{start_token}, matcher_{std::move(matcher)}, arms_{std::move(arms)},
          catch_all_{std::move(catch_all)} {};

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<MatchExpression>, ParserDiagnostic>;

    [[nodiscard]] auto matcher() const noexcept -> const Expression& { return *matcher_; }
    [[nodiscard]] auto arms() const noexcept -> std::span<const MatchArm> { return arms_; }
    [[nodiscard]] auto catch_all() const noexcept -> Optional<const Statement&> {
        return catch_all_ ? Optional<const Statement&>{**catch_all_} : nullopt;
    }

  private:
    std::unique_ptr<Expression>          matcher_;
    std::vector<MatchArm>                arms_;
    Optional<std::unique_ptr<Statement>> catch_all_;
};

} // namespace ast
