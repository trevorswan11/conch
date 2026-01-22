#pragma once

#include <memory>
#include <string>
#include <utility>

#include "core.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace ast {

class IdentifierExpression : public Expression {
  public:
    explicit IdentifierExpression(const Token& start_token, std::string name) noexcept
        : Expression{start_token}, name_{std::move(name)} {}

    auto accept(Visitor& v) const -> void override;

    static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<IdentifierExpression>, ParserDiagnostic>;

  private:
    std::string name_;
};

} // namespace ast
