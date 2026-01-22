#pragma once

#include <memory>
#include <optional>

#include "core.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace ast {

class IdentifierExpression;
class TypeExpression;

struct StructMember {
    std::unique_ptr<IdentifierExpression>      name;
    std::unique_ptr<TypeExpression>            type;
    std::optional<std::unique_ptr<Expression>> default_value{};
};

class StructExpression : public Expression {
  public:
    explicit StructExpression(const Token&                          start_token,
                              std::unique_ptr<IdentifierExpression> name,
                              std::vector<StructMember>             members) noexcept;

    auto accept(Visitor& v) -> void override;

    static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<StructExpression>, ParserDiagnostic>;

  private:
    std::unique_ptr<IdentifierExpression> name_;
    std::vector<StructMember>             members_;
};

} // namespace ast
