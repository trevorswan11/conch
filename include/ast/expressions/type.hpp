#pragma once

#include <memory>
#include <optional>
#include <variant>
#include <vector>

#include "core.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace ast {

class IdentifierExpression;
class TypeExpression;
class FunctionExpression;

using ExplicitIdentType    = std::unique_ptr<IdentifierExpression>;
using ExplicitReferredType = std::unique_ptr<Expression>;
using ExplicitFunctionType = std::unique_ptr<FunctionExpression>;

struct ExplicitArrayType {
    ~ExplicitArrayType();

    std::vector<size_t>             dimensions;
    std::unique_ptr<TypeExpression> inner_type;
};

enum class ExplicitTypeConstraint {
    PRIMITIVE,
    GENERIC_TYPE,
};

struct ExplicitType {
    ~ExplicitType();

    std::variant<ExplicitIdentType, ExplicitReferredType, ExplicitFunctionType, ExplicitArrayType>
                                          type;
    bool                                  nullable;
    std::optional<ExplicitTypeConstraint> constraint{};
};

class TypeExpression : public Expression {
  public:
    explicit TypeExpression(const Token& start_token) noexcept;
    explicit TypeExpression(const Token& start_token, std::optional<ExplicitType> exp) noexcept;
    ~TypeExpression();

    auto accept(Visitor& v) const -> void override;

    static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<TypeExpression>, ParserDiagnostic>;

  private:
    std::optional<ExplicitType> explicit_;
};

} // namespace ast
