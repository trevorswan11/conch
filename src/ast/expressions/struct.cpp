#include <utility>

#include "ast/expressions/struct.hpp"

#include "ast/expressions/function.hpp"
#include "ast/expressions/identifier.hpp"
#include "ast/expressions/type.hpp"
#include "ast/statements/decl.hpp"

#include "visitor/visitor.hpp"

namespace conch::ast {

StructMember::StructMember(bool                      priv,
                           Box<IdentifierExpression> name,
                           Box<TypeExpression>       type,
                           Optional<Box<Expression>> default_value) noexcept
    : private_{priv}, name_{std::move(name)}, type_{std::move(type)},
      default_value_{std::move(default_value)} {}

StructMember::~StructMember() = default;

StructExpression::StructExpression(const Token&                         start_token,
                                   bool                                 packed,
                                   std::vector<Box<DeclStatement>>      declarations,
                                   std::vector<StructMember>            members,
                                   std::vector<Box<FunctionExpression>> functions) noexcept
    : Expression{start_token}, packed_{packed}, declarations_{std::move(declarations)},
      members_{std::move(members)}, functions_{std::move(functions)} {}

StructExpression::~StructExpression() = default;

auto StructExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto StructExpression::parse(Parser& parser) -> Expected<Box<StructExpression>, ParserDiagnostic> {
    TODO(parser);
}

} // namespace conch::ast
