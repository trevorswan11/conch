#include <assert.h>

#include "ast/ast.h"
#include "ast/expressions/index.h"

#include "semantic/context.h"
#include "semantic/type.h"

#include "util/containers/string_builder.h"

[[nodiscard]] Status index_expression_create(Token             start_token,
                                             Expression*       array,
                                             Expression*       idx,
                                             IndexExpression** index_expr,
                                             memory_alloc_fn   memory_alloc) {
    assert(memory_alloc);
    ASSERT_EXPRESSION(array);
    ASSERT_EXPRESSION(idx);

    IndexExpression* index = memory_alloc(sizeof(IndexExpression));
    if (!index) { return ALLOCATION_FAILED; }

    *index = (IndexExpression){
        .base  = EXPRESSION_INIT(INDEX_VTABLE, start_token),
        .array = array,
        .idx   = idx,
    };

    *index_expr = index;
    return SUCCESS;
}

void index_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    if (!node) { return; }
    assert(free_alloc);

    IndexExpression* index = (IndexExpression*)node;
    NODE_VIRTUAL_FREE(index->array, free_alloc);
    NODE_VIRTUAL_FREE(index->idx, free_alloc);

    free_alloc(index);
}

[[nodiscard]] Status
index_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    ASSERT_EXPRESSION(node);
    assert(sb);

    IndexExpression* index = (IndexExpression*)node;
    ASSERT_EXPRESSION(index->array);
    TRY(NODE_VIRTUAL_RECONSTRUCT(index->array, symbol_map, sb));

    TRY(string_builder_append(sb, '['));
    ASSERT_EXPRESSION(index->idx);
    TRY(NODE_VIRTUAL_RECONSTRUCT(index->idx, symbol_map, sb));
    TRY(string_builder_append(sb, ']'));

    return SUCCESS;
}

[[nodiscard]] Status
index_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors) {
    ASSERT_EXPRESSION(node);
    assert(parent && errors);

    const Token     start_token = node->start_token;
    const Allocator allocator   = semantic_context_allocator(parent);

    IndexExpression* index = (IndexExpression*)node;
    ASSERT_EXPRESSION(index->array);
    ASSERT_EXPRESSION(index->idx);

    TRY(NODE_VIRTUAL_ANALYZE(index->array, parent, errors));
    SemanticType* array_type = semantic_context_move_analyzed(parent);
    if (array_type->tag != STYPE_ARRAY || array_type->nullable) {
        PUT_STATUS_PROPAGATE(errors,
                             NON_ARRAY_INDEX_TARGET,
                             start_token,
                             RC_RELEASE(array_type, allocator.free_alloc));
    }

    // Indexing is much safer if the index has to be strictly positive and non null
    TRY_DO(NODE_VIRTUAL_ANALYZE(index->idx, parent, errors),
           RC_RELEASE(array_type, allocator.free_alloc));
    SemanticType* idx_type = semantic_context_move_analyzed(parent);
    if (idx_type->tag != STYPE_SIZE_INTEGER || idx_type->nullable) {
        const Token idx_token = NODE_TOKEN(index->idx);
        PUT_STATUS_PROPAGATE(errors, UNEXPECTED_ARRAY_INDEX_TYPE, idx_token, {
            RC_RELEASE(idx_type, allocator.free_alloc);
            RC_RELEASE(array_type, allocator.free_alloc);
        });
    }

    // We need to check the constness of the array, not the inner item
    assert(array_type->variant.array_type);
    SemanticType* resulting_type;
    TRY_DO(
        semantic_type_copy(&resulting_type, array_type->variant.array_type->inner_type, allocator),
        {
            RC_RELEASE(idx_type, allocator.free_alloc);
            RC_RELEASE(array_type, allocator.free_alloc);
        });
    resulting_type->is_const = array_type->is_const;

    parent->analyzed_type = resulting_type;
    RC_RELEASE(idx_type, allocator.free_alloc);
    RC_RELEASE(array_type, allocator.free_alloc);
    return SUCCESS;
}
