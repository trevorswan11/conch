#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "core.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace ast {

struct CallArgument {
    bool                        reference;
    std::unique_ptr<Expression> argument;
};

class CallExpression : public Expression {
  public:
    explicit CallExpression(const Token&                               start_token,
                            std::unique_ptr<Expression>                function,
                            std::vector<std::unique_ptr<CallArgument>> arguments) noexcept
        : Expression{start_token}, function_{std::move(function)},
          arguments_{std::move(arguments)} {}

    auto accept(Visitor& v) const -> void override;

    static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<CallExpression>, ParserDiagnostic>;

  private:
    std::unique_ptr<Expression>                function_;
    std::vector<std::unique_ptr<CallArgument>> arguments_;
};

} // namespace ast
