#include <algorithm>
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
    : Expression{start_token, NodeKind::STRUCT_EXPRESSION}, packed_{packed},
      declarations_{std::move(declarations)}, members_{std::move(members)},
      functions_{std::move(functions)} {}

StructExpression::~StructExpression() = default;

auto StructExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto StructExpression::parse(Parser& parser) -> Expected<Box<StructExpression>, ParserDiagnostic> {
    TODO(parser);
}

auto StructExpression::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted          = as<StructExpression>(other);
    const auto  declarations_eq = std::ranges::equal(
        declarations_, casted.declarations_, [](const auto& a, const auto& b) { return *a == *b; });
    const auto members_eq =
        std::ranges::equal(members_, casted.members_, [](const auto& a, const auto& b) {
            return a.private_ == b.private_ && a.name_ == b.name_ && a.type_ == b.type_ &&
                   optional::unsafe_eq<Expression>(a.default_value_, b.default_value_);
        });
    const auto functions_eq = std::ranges::equal(
        functions_, casted.functions_, [](const auto& a, const auto& b) { return *a == *b; });

    return packed_ == casted.packed_ && declarations_eq && members_eq && functions_eq;
}

} // namespace conch::ast
