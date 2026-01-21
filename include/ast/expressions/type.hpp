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
class StructExpression;
class EnumExpression;
class ArrayExpression;

using ExplicitIdentType    = std::unique_ptr<IdentifierExpression>;
using ExplicitReferredType = std::unique_ptr<Expression>;
using ExplicitStructType   = std::unique_ptr<StructExpression>;
using ExplicitEnumType     = std::unique_ptr<EnumExpression>;

struct ExplicitFunctionType {
    // std::vector<Parameter> parameters;
    std::unique_ptr<TypeExpression> return_type;
};

struct ExplicitArrayType {
    std::vector<size_t>             dimensions;
    std::unique_ptr<TypeExpression> inner_type;
};

struct ExplicitType {
    std::variant<ExplicitIdentType,
                 ExplicitReferredType,
                 ExplicitStructType,
                 ExplicitEnumType,
                 ExplicitFunctionType,
                 ExplicitArrayType>
         type;
    bool nullable;
    bool primitive;
};

class TypeExpression : public Expression {
  public:
    explicit TypeExpression(const Token& start_token) noexcept;
    explicit TypeExpression(const Token& start_token, std::optional<ExplicitType> exp) noexcept;

    auto accept(Visitor& v) -> void override;

    static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<TypeExpression>, ParserDiagnostic>;

  private:
    std::optional<ExplicitType> explicit_;
};

} // namespace ast
