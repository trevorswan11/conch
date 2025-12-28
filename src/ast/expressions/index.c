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
                                             Allocator*        allocator) {
    ASSERT_ALLOCATOR_PTR(allocator);
    ASSERT_EXPRESSION(array);
    ASSERT_EXPRESSION(idx);

    IndexExpression* index = ALLOCATOR_PTR_MALLOC(allocator, sizeof(IndexExpression));
    if (!index) { return ALLOCATION_FAILED; }

    *index = (IndexExpression){
        .base  = EXPRESSION_INIT(INDEX_VTABLE, start_token),
        .array = array,
        .idx   = idx,
    };

    *index_expr = index;
    return SUCCESS;
}

void index_expression_destroy(Node* node, Allocator* allocator) {
    if (!node) { return; }
    ASSERT_ALLOCATOR_PTR(allocator);

    IndexExpression* index = (IndexExpression*)node;
    NODE_VIRTUAL_FREE(index->array, allocator);
    NODE_VIRTUAL_FREE(index->idx, allocator);

    ALLOCATOR_PTR_FREE(allocator, index);
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

    const Token start_token = node->start_token;
    Allocator*  allocator   = semantic_context_allocator(parent);

    IndexExpression* index = (IndexExpression*)node;
    ASSERT_EXPRESSION(index->array);
    ASSERT_EXPRESSION(index->idx);

    TRY(NODE_VIRTUAL_ANALYZE(index->array, parent, errors));
    SemanticType* array_type = semantic_context_move_analyzed(parent);
    if (array_type->tag != STYPE_ARRAY || array_type->nullable) {
        PUT_STATUS_PROPAGATE(
            errors, NON_ARRAY_INDEX_TARGET, start_token, RC_RELEASE(array_type, allocator));
    }

    // Indexing is much safer if the index has to be strictly positive and non null
    TRY_DO(NODE_VIRTUAL_ANALYZE(index->idx, parent, errors), RC_RELEASE(array_type, allocator));
    SemanticType* idx_type  = semantic_context_move_analyzed(parent);
    const Token   idx_token = NODE_TOKEN(index->idx);
    if (idx_type->nullable) {
        PUT_STATUS_PROPAGATE(errors, UNEXPECTED_ARRAY_INDEX_TYPE, idx_token, {
            RC_RELEASE(idx_type, allocator);
            RC_RELEASE(array_type, allocator);
        });
    }

    // Index expressions can be used to get slices from an array and must be explicit
    assert(array_type->variant.array_type);
    SemanticType* resulting_type = nullptr;
    switch (idx_type->tag) {
    case STYPE_SIZE_INTEGER:
        // Standard indexing behavior changes based on the array type
        switch (array_type->variant.array_type->tag) {
        case STYPE_ARRAY_SINGLE_DIM:
        case STYPE_ARRAY_RANGE:
            TRY_DO(semantic_type_copy(
                       &resulting_type, array_type->variant.array_type->inner_type, allocator),
                   {
                       RC_RELEASE(idx_type, allocator);
                       RC_RELEASE(array_type, allocator);
                   });
            break;
        case STYPE_ARRAY_MULTI_DIM:
            // This gets narrowed in by a single dimension into a new array
            TRY_DO(semantic_type_create(&resulting_type, allocator), {
                RC_RELEASE(idx_type, allocator);
                RC_RELEASE(array_type, allocator);
            });

            resulting_type->tag      = STYPE_ARRAY;
            resulting_type->is_const = array_type->variant.array_type->inner_type->is_const;
            resulting_type->nullable = array_type->variant.array_type->inner_type->nullable;
            resulting_type->valued   = array_type->variant.array_type->inner_type->valued;

            SemanticType* inner_type;
            TRY_DO(semantic_type_copy(
                       &inner_type, array_type->variant.array_type->inner_type, allocator),
                   {
                       RC_RELEASE(resulting_type, allocator);
                       RC_RELEASE(idx_type, allocator);
                       RC_RELEASE(array_type, allocator);
                   });

            const ArrayList* old_dimensions = &array_type->variant.array_type->variant.dimensions;
            const size_t     new_dim_count  = old_dimensions->length - 1;

            // The iterator should advance a single position in both cases
            ArrayListConstIterator it = array_list_const_iterator_init(old_dimensions);
            it.index += 1;
            size_t dim;

            SemanticArrayType* new_array;
            if (new_dim_count == 1) {
                UNREACHABLE_IF_ERROR(!array_list_const_iterator_has_next(&it, &dim));
                TRY_DO(semantic_array_create(STYPE_ARRAY_SINGLE_DIM,
                                             (SemanticArrayUnion){.length = dim},
                                             inner_type,
                                             &new_array,
                                             allocator),
                       {
                           RC_RELEASE(inner_type, allocator);
                           RC_RELEASE(resulting_type, allocator);
                           RC_RELEASE(idx_type, allocator);
                           RC_RELEASE(array_type, allocator);
                       });
            } else {
                ArrayList new_dimensions;
                TRY_DO(array_list_init_allocator(
                           &new_dimensions, new_dim_count, sizeof(size_t), allocator),
                       {
                           RC_RELEASE(inner_type, allocator);
                           RC_RELEASE(resulting_type, allocator);
                           RC_RELEASE(idx_type, allocator);
                           RC_RELEASE(array_type, allocator);
                       });

                while (array_list_const_iterator_has_next(&it, &dim)) {
                    array_list_push_assume_capacity(&new_dimensions, &dim);
                }

                TRY_DO(semantic_array_create(STYPE_ARRAY_MULTI_DIM,
                                             (SemanticArrayUnion){.dimensions = new_dimensions},
                                             inner_type,
                                             &new_array,
                                             allocator),
                       {
                           RC_RELEASE(inner_type, allocator);
                           RC_RELEASE(resulting_type, allocator);
                           RC_RELEASE(idx_type, allocator);
                           RC_RELEASE(array_type, allocator);
                       });
            }

            resulting_type->variant = (SemanticTypeUnion){.array_type = new_array};
            break;
        }

        break;
    case STYPE_ARRAY:
        // Ranged arrays will always return the inner array type
        assert(idx_type->variant.array_type);
        if (idx_type->variant.array_type->tag == STYPE_ARRAY_RANGE) {
            if (idx_type->variant.array_type->inner_type->tag != STYPE_SIZE_INTEGER) {
                PUT_STATUS_PROPAGATE(errors, TYPE_MISMATCH, idx_token, {
                    RC_RELEASE(idx_type, allocator);
                    RC_RELEASE(array_type, allocator);
                });
            }

            TRY_DO(semantic_type_copy(&resulting_type, array_type, allocator), {
                RC_RELEASE(idx_type, allocator);
                RC_RELEASE(array_type, allocator);
            });
            break;
        }

        PUT_STATUS_PROPAGATE(errors, UNEXPECTED_ARRAY_INDEX_TYPE, idx_token, {
            RC_RELEASE(idx_type, allocator);
            RC_RELEASE(array_type, allocator);
        });
        break;
    default:
        PUT_STATUS_PROPAGATE(errors, UNEXPECTED_ARRAY_INDEX_TYPE, idx_token, {
            RC_RELEASE(idx_type, allocator);
            RC_RELEASE(array_type, allocator);
        });
    }

    // We need to check the constness of the array, not the inner item
    resulting_type->is_const = array_type->is_const;
    parent->analyzed_type    = resulting_type;

    RC_RELEASE(idx_type, allocator);
    RC_RELEASE(array_type, allocator);
    return SUCCESS;
}
