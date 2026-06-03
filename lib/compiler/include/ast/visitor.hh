#pragma once

// IWYU pragma: begin_keep
#include "ast/expression.hh"
#include "ast/id.hh"
#include "ast/kind.hh"
#include "ast/primitive.hh"
#include "ast/statement.hh"
#include "ast/type.hh"
// IWYU pragma: end_keep

#include "variant.hh"

namespace porpoise::ast {

#define X(Type) Type,
using NodeData = std::variant<FOREACH_AST_NODE(X) Unit>;
#undef X

#define AST_NODE_VISITOR_DEF_GEN_X(NodeType) \
    auto visit(porpoise::ast::NodeID, const porpoise::ast::NodeType&) -> void;
#define AST_NODE_VISITOR_DEF_GEN()               \
    FOREACH_AST_NODE(AST_NODE_VISITOR_DEF_GEN_X) \
    auto visit(porpoise::ast::NodeID, const porpoise::Unit&) -> void;

#define AST_NODE_VISITOR_NOOP(Class, NodeType) \
    auto Class::visit(porpoise::ast::NodeID, const porpoise::ast::NodeType&) -> void {}

#define AST_TYPE_VISITOR_DEF_GEN_X(NodeType) \
    auto visit(porpoise::ast::ExplicitTypeID, const porpoise::ast::NodeType&) -> void;
#define AST_TYPE_VISITOR_DEF_GEN() FOREACH_AST_TYPE(AST_TYPE_VISITOR_DEF_GEN_X)

#define AST_TYPE_VISITOR_NOOP(Class, NodeType) \
    auto Class::visit(porpoise::ast::ExplicitTypeID, const porpoise::ast::NodeType&) -> void {}

using TypeData = std::variant<IdentifierExpression,
                              ModuleAccessExpression,
                              DotExpression,
                              CallExpression,
                              ExplicitFunctionType,
                              ExplicitTypeID,
                              StructExpression,
                              EnumExpression,
                              UnionExpression,
                              ExplicitArrayType>;

#define AST_VISITOR_DEF_GEN()  \
    AST_NODE_VISITOR_DEF_GEN() \
    AST_TYPE_VISITOR_DEF_GEN()

// Creates the template instantiation for a ID-templated, Node/ExplicitType ID visitor
#define VISITOR_TEMPLATE_INIT(ClassName, fn_name, NodeType)             \
    template auto ClassName::fn_name<porpoise::ast::NodeID>(            \
        porpoise::ast::NodeID, const porpoise::ast::NodeType&) -> void; \
    template auto ClassName::fn_name<porpoise::ast::ExplicitTypeID>(    \
        porpoise::ast::ExplicitTypeID, const porpoise::ast::NodeType&) -> void;

} // namespace porpoise::ast
