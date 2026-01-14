#include <assert.h>

#include "ast/ast.h"
#include "ast/expressions/array.h"

#include "semantic/context.h"
#include "semantic/type.h"

#include "util/containers/array_list.h"
#include "util/containers/string_builder.h"

[[nodiscard]] Status array_literal_expression_create(Token                    start_token,
                                                     bool                     inferred_size,
                                                     ArrayList                items,
                                                     ArrayLiteralExpression** array_expr,
                                                     Allocator*               allocator) {
    ASSERT_ALLOCATOR_PTR(allocator);
    assert(items.item_size == sizeof(Expression*));

    ArrayLiteralExpression* array = ALLOCATOR_PTR_MALLOC(allocator, sizeof(*array));
    if (!array) { return ALLOCATION_FAILED; }

    *array = (ArrayLiteralExpression){
        .base          = EXPRESSION_INIT(ARRAY_VTABLE, start_token),
        .inferred_size = inferred_size,
        .items         = items,
    };

    *array_expr = array;
    return SUCCESS;
}

void array_literal_expression_destroy(Node* node, Allocator* allocator) {
    if (!node) { return; }
    ASSERT_ALLOCATOR_PTR(allocator);

    ArrayLiteralExpression* array = (ArrayLiteralExpression*)node;
    free_expression_list(&array->items, allocator);

    ALLOCATOR_PTR_FREE(allocator, array);
}

[[nodiscard]] Status
array_literal_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    ASSERT_EXPRESSION(node);
    assert(sb);

    ArrayLiteralExpression* array = (ArrayLiteralExpression*)node;
    TRY(string_builder_append(sb, '['));
    if (array->inferred_size) {
        TRY(string_builder_append(sb, '_'));
    } else {
        TRY(string_builder_append_unsigned(sb, (uint64_t)array->items.length));
        TRY(string_builder_append_str_z(sb, "uz"));
    }
    TRY(string_builder_append_str_z(sb, "]{ "));

    ArrayListConstIterator it = array_list_const_iterator_init(&array->items);
    Expression*            item;
    while (array_list_const_iterator_has_next(&it, (void*)&item)) {
        ASSERT_EXPRESSION(item);
        TRY(NODE_VIRTUAL_RECONSTRUCT(item, symbol_map, sb));
        TRY(string_builder_append_str_z(sb, ", "));
    }

    TRY(string_builder_append(sb, '}'));
    return SUCCESS;
}

[[nodiscard]] Status
array_literal_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors) {
    ASSERT_EXPRESSION(node);
    assert(parent && errors);

    // Explicitly sized arrays have their argument number enforced by the size
    ArrayLiteralExpression* array = (ArrayLiteralExpression*)node;
    assert(array->items.data && array->items.length > 0);

    const Token start_token = node->start_token;
    Allocator*  allocator   = semantic_context_allocator(parent);

    ArrayListConstIterator it = array_list_const_iterator_init(&array->items);
    Expression*            item;
    UNREACHABLE_IF(!array_list_const_iterator_has_next(&it, (void*)&item));

    TRY(NODE_VIRTUAL_ANALYZE(item, parent, errors));
    SemanticType* inner_type = semantic_context_move_analyzed(parent);
    if (!inner_type->valued) {
        PUT_STATUS_PROPAGATE(
            errors, NON_VALUED_ARRAY_ITEM, start_token, RC_RELEASE(inner_type, allocator));
    }

    // All following types must be exactly equal to the first type
    while (array_list_const_iterator_has_next(&it, (void*)&item)) {
        TRY_DO(NODE_VIRTUAL_ANALYZE(item, parent, errors), RC_RELEASE(inner_type, allocator));
        SemanticType* next_type = semantic_context_move_analyzed(parent);

        if (!type_equal(inner_type, next_type)) {
            PUT_STATUS_PROPAGATE(errors, ARRAY_ITEM_TYPE_MISMATCH, start_token, {
                RC_RELEASE(inner_type, allocator);
                RC_RELEASE(next_type, allocator);
            });
        }

        RC_RELEASE(next_type, allocator);
    }

    SemanticType* resulting_type;
    TRY_DO(semantic_type_create(&resulting_type, allocator), RC_RELEASE(inner_type, allocator));
    resulting_type->is_const = false;
    resulting_type->valued   = true;
    resulting_type->nullable = false;

    // The type of the semantic array is dependent on the inner type
    const size_t       num_items = array->items.length;
    SemanticArrayType* sarray;
    if (inner_type->tag == STYPE_ARRAY) {
        SemanticArrayType* inner_array_type = inner_type->variant.array_type;
        ArrayList          resulting_dimensions;

        switch (inner_array_type->tag) {
        // [_]{ [_]{1, }, ... }
        case STYPE_ARRAY_SINGLE_DIM:
            // The dimensions (2D) have to be [outer count, inner array length]
            TRY_DO(array_list_init_allocator(&resulting_dimensions, 2, sizeof(size_t), allocator), {
                RC_RELEASE(inner_type, allocator);
                RC_RELEASE(resulting_type, allocator);
            });

            array_list_push_assume_capacity(&resulting_dimensions, &num_items);
            array_list_push_assume_capacity(&resulting_dimensions,
                                            &inner_array_type->variant.length);
            break;
        // [_]{ [2uz, 3uz]{[_]{...} }, ... }
        case STYPE_ARRAY_MULTI_DIM:
            // The dimensions (3D+) have to be [outer count, lengths of multi]
            const ArrayList* multi_lengths = &inner_array_type->variant.dimensions;
            TRY_DO(array_list_copy_from(&resulting_dimensions, multi_lengths), {
                RC_RELEASE(inner_type, allocator);
                RC_RELEASE(resulting_type, allocator);
            });

            TRY_DO(array_list_ensure_total_capacity(&resulting_dimensions,
                                                    resulting_dimensions.capacity + 1),
                   {
                       array_list_deinit(&resulting_dimensions);
                       RC_RELEASE(inner_type, allocator);
                       RC_RELEASE(resulting_type, allocator);
                   });
            array_list_insert_stable_assume_capacity(&resulting_dimensions, 0, &num_items);
            break;
        // [_]{ 0uz..6uz, }, ... }
        case STYPE_ARRAY_RANGE:
            // Error out since we cannot determine the length of a range expression at compile time
            PUT_STATUS_PROPAGATE(errors, ILLEGAL_ARRAY_ITEM, start_token, {
                RC_RELEASE(inner_type, allocator);
                RC_RELEASE(resulting_type, allocator);
            });
        }

        SemanticType* actual_inner;
        TRY_DO(semantic_type_copy(&actual_inner, inner_array_type->inner_type, allocator), {
            array_list_deinit(&resulting_dimensions);
            RC_RELEASE(inner_type, allocator);
            RC_RELEASE(resulting_type, allocator);
        });

        TRY_DO(semantic_array_create(STYPE_ARRAY_MULTI_DIM,
                                     (SemanticArrayUnion){.dimensions = resulting_dimensions},
                                     actual_inner,
                                     &sarray,
                                     allocator),
               {
                   RC_RELEASE(actual_inner, allocator);
                   array_list_deinit(&resulting_dimensions);
                   RC_RELEASE(inner_type, allocator);
                   RC_RELEASE(resulting_type, allocator);
               });
    } else {
        TRY_DO(semantic_array_create(STYPE_ARRAY_SINGLE_DIM,
                                     (SemanticArrayUnion){.length = array->items.length},
                                     rc_retain(inner_type),
                                     &sarray,
                                     allocator),
               {
                   RC_RELEASE(inner_type, allocator);
                   RC_RELEASE(resulting_type, allocator);
               });
    }

    // Everything allocated is moved into the resulting type
    resulting_type->tag     = STYPE_ARRAY;
    resulting_type->variant = (SemanticTypeUnion){.array_type = sarray};
    parent->analyzed_type   = resulting_type;
    RC_RELEASE(inner_type, allocator);
    return SUCCESS;
}
