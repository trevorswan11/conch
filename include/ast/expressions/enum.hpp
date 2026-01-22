#pragma once

#include <memory>
#include <optional>

#include "core.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace ast {

class IdentifierExpression;

struct EnumVariant {
    std::unique_ptr<IdentifierExpression>      name;
    std::optional<std::unique_ptr<Expression>> value;
};

class EnumExpression : public Expression {
  public:
    explicit EnumExpression(const Token&                          start_token,
                            std::unique_ptr<IdentifierExpression> name,
                            std::vector<EnumVariant>              variants) noexcept;
    ~EnumExpression();

    auto accept(Visitor& v) const -> void override;

    static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<EnumExpression>, ParserDiagnostic>;

  private:
    std::unique_ptr<IdentifierExpression> name_;
    std::vector<EnumVariant>              variants_;
};

} // namespace ast
