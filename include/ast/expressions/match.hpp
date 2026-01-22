#pragma once

#include <memory>
#include <optional>
#include <utility>

#include "core.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace ast {

struct MatchArm {
    std::unique_ptr<Expression> pattern;
    std::unique_ptr<Statement>  dispatch;
};

class MatchExpression : public Expression {
  public:
    explicit MatchExpression(const Token&                              start_token,
                             std::unique_ptr<Expression>               matcher,
                             std::vector<MatchArm>                     arms,
                             std::optional<std::unique_ptr<Statement>> catch_all) noexcept
        : Expression{start_token}, matcher_{std::move(matcher)}, arms_{std::move(arms)},
          catch_all_{std::move(catch_all)} {};

    auto accept(Visitor& v) -> void override;

    static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<MatchExpression>, ParserDiagnostic>;

  private:
    std::unique_ptr<Expression>               matcher_;
    std::vector<MatchArm>                     arms_;
    std::optional<std::unique_ptr<Statement>> catch_all_;
};

} // namespace ast
