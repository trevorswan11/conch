#pragma once

#include <variant>

#include "util/common.hpp"
#include "util/expected.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class IdentifierExpression;
class ScopeResolutionExpression;

class NamespaceStatement : public Statement {
  public:
    using Default = Box<IdentifierExpression>;
    using Nested  = Box<ScopeResolutionExpression>;

  public:
    explicit NamespaceStatement(const Token&                  start_token,
                                std::variant<Default, Nested> nspace) noexcept;
    ~NamespaceStatement() override;

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<Box<NamespaceStatement>, ParserDiagnostic>;

  private:
    std::variant<Default, Nested> namespace_;
};

} // namespace conch::ast
