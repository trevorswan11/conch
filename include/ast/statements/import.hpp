#pragma once

#include <memory>
#include <optional>
#include <variant>

#include "core.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace ast {

class IdentifierExpression;
class StringExpression;

class ImportStatement : public Statement {
  public:
    explicit ImportStatement(const Token&                                         start_token,
                             std::variant<std::unique_ptr<IdentifierExpression>,
                                          std::unique_ptr<StringExpression>>      import,
                             std::optional<std::unique_ptr<IdentifierExpression>> alias) noexcept;

    auto accept(Visitor& v) -> void override;

    static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<ImportStatement>, ParserDiagnostic>;

  private:
    std::variant<std::unique_ptr<IdentifierExpression>, std::unique_ptr<StringExpression>> import_;
    std::optional<std::unique_ptr<IdentifierExpression>>                                   alias_;
};

} // namespace ast
