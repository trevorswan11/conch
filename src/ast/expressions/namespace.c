#include <assert.h>

#include "ast/expressions/identifier.h"
#include "ast/expressions/namespace.h"

#include "semantic/context.h"
#include "semantic/type.h"

#include "util/containers/string_builder.h"

[[nodiscard]] Status namespace_expression_create(Token                 start_token,
                                                 Expression*           outer,
                                                 IdentifierExpression* inner,
                                                 NamespaceExpression** namespace_expr,
                                                 Allocator*            allocator) {
    ASSERT_ALLOCATOR_PTR(allocator);
    ASSERT_EXPRESSION(outer);
    ASSERT_EXPRESSION(inner);

    NamespaceExpression* namespace_local =
        ALLOCATOR_PTR_MALLOC(allocator, sizeof(NamespaceExpression));
    if (!namespace_local) { return ALLOCATION_FAILED; }

    *namespace_local = (NamespaceExpression){
        .base  = EXPRESSION_INIT(NAMESPACE_VTABLE, start_token),
        .outer = outer,
        .inner = inner,
    };

    *namespace_expr = namespace_local;
    return SUCCESS;
}

void namespace_expression_destroy(Node* node, Allocator* allocator) {
    if (!node) { return; }
    ASSERT_ALLOCATOR_PTR(allocator);

    NamespaceExpression* namespace_expr = (NamespaceExpression*)node;
    NODE_VIRTUAL_FREE(namespace_expr->outer, allocator);
    NODE_VIRTUAL_FREE(namespace_expr->inner, allocator);

    ALLOCATOR_PTR_FREE(allocator, namespace_expr);
}

[[nodiscard]] Status
namespace_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    ASSERT_EXPRESSION(node);
    assert(sb);

    NamespaceExpression* namespace_expr = (NamespaceExpression*)node;

    ASSERT_EXPRESSION(namespace_expr->outer);
    TRY(NODE_VIRTUAL_RECONSTRUCT(namespace_expr->outer, symbol_map, sb));
    TRY(string_builder_append_str_z(sb, "::"));
    ASSERT_EXPRESSION(namespace_expr->inner);
    TRY(NODE_VIRTUAL_RECONSTRUCT(namespace_expr->inner, symbol_map, sb));

    return SUCCESS;
}

[[nodiscard]] Status
namespace_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors) {
    ASSERT_EXPRESSION(node);
    assert(parent && errors);

    const Token start_token = node->start_token;
    Allocator*  allocator   = semantic_context_allocator(parent);

    NamespaceExpression* namespace_expr = (NamespaceExpression*)node;
    ASSERT_EXPRESSION(namespace_expr->outer);
    ASSERT_EXPRESSION(namespace_expr->inner);

    TRY(NODE_VIRTUAL_ANALYZE(namespace_expr->outer, parent, errors));
    SemanticType* direct_parent = semantic_context_move_analyzed(parent);

    // All we need to check is the inner presence in the namespace
    SemanticType* inner_type;
    TRY_DO(semantic_type_create(&inner_type, allocator), RC_RELEASE(direct_parent, allocator));
    switch (direct_parent->tag) {
    case STYPE_ENUM: {
        const HashSet variants = direct_parent->variant.enum_type->variants;
        if (!hash_set_contains(&variants, &namespace_expr->inner->name)) {
            PUT_STATUS_PROPAGATE(errors, UNKNOWN_ENUM_VARIANT, start_token, {
                RC_RELEASE(direct_parent, allocator);
                RC_RELEASE(inner_type, allocator);
            });
        }

        TRY_DO(semantic_type_copy_variant(inner_type, direct_parent, allocator), {
            RC_RELEASE(direct_parent, allocator);
            RC_RELEASE(inner_type, allocator);
        });

        inner_type->is_const = true;
        inner_type->valued   = true;
        inner_type->nullable = false;
        break;
    }
    default:
        PUT_STATUS_PROPAGATE(errors, ILLEGAL_OUTER_NAMESPACE, start_token, {
            RC_RELEASE(direct_parent, allocator);
            RC_RELEASE(inner_type, allocator);
        });
    }

    // The parent type doesn't matter for the resulting type
    RC_RELEASE(direct_parent, allocator);
    parent->analyzed_type = inner_type;
    return SUCCESS;
}
