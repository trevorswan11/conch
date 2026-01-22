#pragma once

#include <memory>

#include "core.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace ast {

class IdentifierExpression;
class BlockStatement;

class ImplStatement : public Statement {
  public:
    explicit ImplStatement(const Token&                          start_token,
                           std::unique_ptr<IdentifierExpression> parent,
                           std::unique_ptr<BlockStatement>       implementation) noexcept;
    ~ImplStatement();

    auto accept(Visitor& v) const -> void override;

    static auto parse(Parser& parser) -> Expected<std::unique_ptr<ImplStatement>, ParserDiagnostic>;

  private:
    std::unique_ptr<IdentifierExpression> parent_;
    std::unique_ptr<BlockStatement>       implementation_;
};

} // namespace ast
