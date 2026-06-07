#pragma once

#include <concepts>

#include "types.hh"
#include "variant.hh"

namespace ghoti {

namespace ast {

enum class NodeKind : u8 {
    ARRAY_EXPRESSION,
    CALL_EXPRESSION,
    DO_WHILE_LOOP_EXPRESSION,
    ENUM_EXPRESSION,
    FOR_LOOP_EXPRESSION,
    FUNCTION_EXPRESSION,
    IDENTIFIER_EXPRESSION,
    IF_EXPRESSION,
    INDEX_EXPRESSION,
    INFINITE_LOOP_EXPRESSION,
    ASSIGNMENT_EXPRESSION,
    BINARY_EXPRESSION,
    DOT_EXPRESSION,
    RANGE_EXPRESSION,
    INITIALIZER_EXPRESSION,
    LABEL_EXPRESSION,
    MATCH_EXPRESSION,
    UNARY_EXPRESSION,
    REFERENCE_EXPRESSION,
    ADDRESS_OF_EXPRESSION,
    DEREFERENCE_EXPRESSION,
    IMPLICIT_ACCESS_EXPRESSION,
    STRING_EXPRESSION,
    I32_EXPRESSION,
    I64_EXPRESSION,
    ISIZE_EXPRESSION,
    U32_EXPRESSION,
    U64_EXPRESSION,
    USIZE_EXPRESSION,
    U8_EXPRESSION,
    F32_EXPRESSION,
    F64_EXPRESSION,
    BOOL_EXPRESSION,
    VOID_EXPRESSION,
    UNDEFINED_EXPRESSION,
    MODULE_ACCESS_EXPRESSION,
    STRUCT_EXPRESSION,
    UNION_EXPRESSION,
    WHILE_LOOP_EXPRESSION,

    BLOCK_STATEMENT,
    BREAK_STATEMENT,
    CONTINUE_STATEMENT,
    DECL_STATEMENT,
    DEFER_STATEMENT,
    DISCARD_STATEMENT,
    EXPRESSION_STATEMENT,
    IMPORT_STATEMENT,
    RETURN_STATEMENT,
    TEST_STATEMENT,
    USING_STATEMENT,

    DISCARDED, // Represented by Unit
};

#define FOREACH_AST_EXPR(X)     \
    X(ArrayExpression)          \
    X(CallExpression)           \
    X(DoWhileLoopExpression)    \
    X(EnumExpression)           \
    X(ForLoopExpression)        \
    X(FunctionExpression)       \
    X(IdentifierExpression)     \
    X(IfExpression)             \
    X(IndexExpression)          \
    X(InfiniteLoopExpression)   \
    X(AssignmentExpression)     \
    X(BinaryExpression)         \
    X(DotExpression)            \
    X(RangeExpression)          \
    X(InitializerExpression)    \
    X(LabelExpression)          \
    X(MatchExpression)          \
    X(ReferenceExpression)      \
    X(AddressOfExpression)      \
    X(DereferenceExpression)    \
    X(UnaryExpression)          \
    X(ImplicitAccessExpression) \
    X(StringExpression)         \
    X(I32Expression)            \
    X(I64Expression)            \
    X(ISizeExpression)          \
    X(U32Expression)            \
    X(U64Expression)            \
    X(USizeExpression)          \
    X(U8Expression)             \
    X(F32Expression)            \
    X(F64Expression)            \
    X(BoolExpression)           \
    X(VoidExpression)           \
    X(UndefinedExpression)      \
    X(ModuleAccessExpression)   \
    X(StructExpression)         \
    X(UnionExpression)          \
    X(WhileLoopExpression)

#define FOREACH_AST_STMT(X) \
    X(BlockStatement)       \
    X(BreakStatement)       \
    X(ContinueStatement)    \
    X(DeclStatement)        \
    X(DeferStatement)       \
    X(DiscardStatement)     \
    X(ExpressionStatement)  \
    X(ImportStatement)      \
    X(ReturnStatement)      \
    X(TestStatement)        \
    X(UsingStatement)

// DOES NOT INCLUDE DISCARDED
#define FOREACH_AST_NODE(X) \
    FOREACH_AST_EXPR(X)     \
    FOREACH_AST_STMT(X)

using Discarded = Unit;

#define FWD_DECLARE_NODE_X(NodeType) struct NodeType;
FOREACH_AST_NODE(FWD_DECLARE_NODE_X)
#undef FWD_DECLARE_NODE_X

enum class ExplicitTypeKind : u8 {
    IDENT,
    SCOPE,
    DOT,
    CALL,
    FUNCTION,
    RECURSIVE,
    STRUCT,
    ENUM,
    UNION,
    ARRAY,
};

#define FOREACH_AST_TYPE(X)   \
    X(IdentifierExpression)   \
    X(ModuleAccessExpression) \
    X(DotExpression)          \
    X(CallExpression)         \
    X(ExplicitFunctionType)   \
    X(ExplicitTypeID)         \
    X(StructExpression)       \
    X(EnumExpression)         \
    X(UnionExpression)        \
    X(ExplicitArrayType)

class ExplicitTypeID;
struct ExplicitArrayType;
struct ExplicitFunctionType;

} // namespace ast

namespace traits {

template <typename T> struct NodeKindOf;

template <typename T>
concept ASTNode = requires {
    { NodeKindOf<T>::value() } -> std::same_as<ast::NodeKind>;
};

#define NODE_KIND_OF_TRAIT(Type, Kind)                                          \
    template <> struct NodeKindOf<ast::Type> {                                  \
        [[nodiscard]] static constexpr auto value() noexcept -> ast::NodeKind { \
            return ast::NodeKind::Kind;                                         \
        }                                                                       \
    };

NODE_KIND_OF_TRAIT(ArrayExpression, ARRAY_EXPRESSION)
NODE_KIND_OF_TRAIT(CallExpression, CALL_EXPRESSION)
NODE_KIND_OF_TRAIT(DoWhileLoopExpression, DO_WHILE_LOOP_EXPRESSION)
NODE_KIND_OF_TRAIT(EnumExpression, ENUM_EXPRESSION)
NODE_KIND_OF_TRAIT(ForLoopExpression, FOR_LOOP_EXPRESSION)
NODE_KIND_OF_TRAIT(FunctionExpression, FUNCTION_EXPRESSION)
NODE_KIND_OF_TRAIT(IdentifierExpression, IDENTIFIER_EXPRESSION)
NODE_KIND_OF_TRAIT(IfExpression, IF_EXPRESSION)
NODE_KIND_OF_TRAIT(IndexExpression, INDEX_EXPRESSION)
NODE_KIND_OF_TRAIT(InfiniteLoopExpression, INFINITE_LOOP_EXPRESSION)
NODE_KIND_OF_TRAIT(AssignmentExpression, ASSIGNMENT_EXPRESSION)
NODE_KIND_OF_TRAIT(BinaryExpression, BINARY_EXPRESSION)
NODE_KIND_OF_TRAIT(DotExpression, DOT_EXPRESSION)
NODE_KIND_OF_TRAIT(RangeExpression, RANGE_EXPRESSION)
NODE_KIND_OF_TRAIT(InitializerExpression, INITIALIZER_EXPRESSION)
NODE_KIND_OF_TRAIT(LabelExpression, LABEL_EXPRESSION)
NODE_KIND_OF_TRAIT(MatchExpression, MATCH_EXPRESSION)
NODE_KIND_OF_TRAIT(UnaryExpression, UNARY_EXPRESSION)
NODE_KIND_OF_TRAIT(ReferenceExpression, REFERENCE_EXPRESSION)
NODE_KIND_OF_TRAIT(DereferenceExpression, DEREFERENCE_EXPRESSION)
NODE_KIND_OF_TRAIT(AddressOfExpression, ADDRESS_OF_EXPRESSION)
NODE_KIND_OF_TRAIT(ImplicitAccessExpression, IMPLICIT_ACCESS_EXPRESSION)
NODE_KIND_OF_TRAIT(StringExpression, STRING_EXPRESSION)
NODE_KIND_OF_TRAIT(I32Expression, I32_EXPRESSION)
NODE_KIND_OF_TRAIT(I64Expression, I64_EXPRESSION)
NODE_KIND_OF_TRAIT(ISizeExpression, ISIZE_EXPRESSION)
NODE_KIND_OF_TRAIT(U32Expression, U32_EXPRESSION)
NODE_KIND_OF_TRAIT(U64Expression, U64_EXPRESSION)
NODE_KIND_OF_TRAIT(USizeExpression, USIZE_EXPRESSION)
NODE_KIND_OF_TRAIT(U8Expression, U8_EXPRESSION)
NODE_KIND_OF_TRAIT(F32Expression, F32_EXPRESSION)
NODE_KIND_OF_TRAIT(F64Expression, F64_EXPRESSION)
NODE_KIND_OF_TRAIT(BoolExpression, BOOL_EXPRESSION)
NODE_KIND_OF_TRAIT(VoidExpression, VOID_EXPRESSION)
NODE_KIND_OF_TRAIT(UndefinedExpression, UNDEFINED_EXPRESSION)
NODE_KIND_OF_TRAIT(ModuleAccessExpression, MODULE_ACCESS_EXPRESSION)
NODE_KIND_OF_TRAIT(StructExpression, STRUCT_EXPRESSION)
NODE_KIND_OF_TRAIT(UnionExpression, UNION_EXPRESSION)
NODE_KIND_OF_TRAIT(WhileLoopExpression, WHILE_LOOP_EXPRESSION)

NODE_KIND_OF_TRAIT(BlockStatement, BLOCK_STATEMENT)
NODE_KIND_OF_TRAIT(BreakStatement, BREAK_STATEMENT)
NODE_KIND_OF_TRAIT(ContinueStatement, CONTINUE_STATEMENT)
NODE_KIND_OF_TRAIT(DeclStatement, DECL_STATEMENT)
NODE_KIND_OF_TRAIT(DeferStatement, DEFER_STATEMENT)
NODE_KIND_OF_TRAIT(DiscardStatement, DISCARD_STATEMENT)
NODE_KIND_OF_TRAIT(ExpressionStatement, EXPRESSION_STATEMENT)
NODE_KIND_OF_TRAIT(ImportStatement, IMPORT_STATEMENT)
NODE_KIND_OF_TRAIT(ReturnStatement, RETURN_STATEMENT)
NODE_KIND_OF_TRAIT(TestStatement, TEST_STATEMENT)
NODE_KIND_OF_TRAIT(UsingStatement, USING_STATEMENT)
NODE_KIND_OF_TRAIT(Discarded, DISCARDED)

#undef KIND_OF_TRAIT

template <typename T> struct ExplicitTypeKindOf;

template <typename T>
concept ASTExplicitType = requires {
    { ExplicitTypeKindOf<T>::value() } -> std::same_as<ast::ExplicitTypeKind>;
};

#define KIND_OF_TRAIT(Type, Kind)                                                       \
    template <> struct ExplicitTypeKindOf<ast::Type> {                                  \
        [[nodiscard]] static constexpr auto value() noexcept -> ast::ExplicitTypeKind { \
            return ast::ExplicitTypeKind::Kind;                                         \
        }                                                                               \
    };

KIND_OF_TRAIT(IdentifierExpression, IDENT)
KIND_OF_TRAIT(ModuleAccessExpression, SCOPE)
KIND_OF_TRAIT(DotExpression, DOT)
KIND_OF_TRAIT(CallExpression, CALL)
KIND_OF_TRAIT(ExplicitFunctionType, FUNCTION)
KIND_OF_TRAIT(ExplicitTypeID, RECURSIVE)
KIND_OF_TRAIT(StructExpression, STRUCT)
KIND_OF_TRAIT(EnumExpression, ENUM)
KIND_OF_TRAIT(UnionExpression, UNION)
KIND_OF_TRAIT(ExplicitArrayType, ARRAY)

#undef KIND_OF_TRAIT

} // namespace traits

} // namespace ghoti
