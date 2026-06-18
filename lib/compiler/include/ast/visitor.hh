#pragma once

#include <stdx/variant.hh>

// IWYU pragma: begin_keep
#include "ast/expression.hh"
#include "ast/id.hh"
#include "ast/kind.hh"
#include "ast/primitive.hh"
#include "ast/statement.hh"
#include "ast/type.hh"
// IWYU pragma: end_keep

namespace ghoti::ast {

#define X(Type) Type,
using NodeData = stdx::Variant<FOREACH_AST_NODE(X) Discarded>;
#undef X

#define AST_NODE_VISITOR_NOOP(Class, NodeType) \
    auto Class::visit(ghoti::ast::NodeID, const ghoti::ast::NodeType&) -> void {}

using TypeData = stdx::Variant<IdentifierExpression,
                               ModuleAccessExpression,
                               DotExpression,
                               CallExpression,
                               ExplicitFunctionType,
                               ExplicitTypeID,
                               StructExpression,
                               EnumExpression,
                               UnionExpression,
                               ExplicitArrayType>;

#define AST_TYPE_VISITOR_NOOP(Class, NodeType) \
    auto Class::visit(ghoti::ast::ExplicitTypeID, const ghoti::ast::NodeType&) -> void {}

// Creates the template instantiation for a ID-templated, Node/ExplicitType ID visitor
#define VISITOR_TEMPLATE_INIT(ClassName, fn_name, NodeType)                                   \
    template auto ClassName::fn_name<ghoti::ast::NodeID>(ghoti::ast::NodeID, NodeType)->void; \
    template auto ClassName::fn_name<ghoti::ast::ExplicitTypeID>(ghoti::ast::ExplicitTypeID,  \
                                                                 NodeType)                    \
        ->void;

} // namespace ghoti::ast
