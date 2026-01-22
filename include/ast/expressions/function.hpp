#pragma once

#include <memory>
#include <optional>

#include "core.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace ast {

class IdentifierExpression;
class TypeExpression;
class BlockStatement;

struct FunctionParameter {
    bool                                       reference;
    std::unique_ptr<IdentifierExpression>      name;
    std::unique_ptr<TypeExpression>            type;
    std::optional<std::unique_ptr<Expression>> default_value{};
};

class FunctionExpression : public Expression {
  public:
    explicit FunctionExpression(const Token&                                   start_token,
                                std::vector<FunctionParameter>                 parameters,
                                std::unique_ptr<TypeExpression>                return_typ,
                                std::optional<std::unique_ptr<BlockStatement>> body) noexcept;
    ~FunctionExpression();

    auto accept(Visitor& v) const -> void override;

    static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<FunctionExpression>, ParserDiagnostic>;

  private:
    std::vector<FunctionParameter>                 parameters_;
    std::unique_ptr<TypeExpression>                return_type;
    std::optional<std::unique_ptr<BlockStatement>> body_;
};

} // namespace ast
