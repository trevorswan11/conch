#include <assert.h>

#include "ast/ast.h"
#include "ast/expressions/array.h"

#include "semantic/context.h"
#include "semantic/type.h"

#include "util/containers/string_builder.h"

NODISCARD Status array_literal_expression_create(Token                    start_token,
                                                 bool                     inferred_size,
                                                 ArrayList                items,
                                                 ArrayLiteralExpression** array_expr,
                                                 memory_alloc_fn          memory_alloc) {
    assert(memory_alloc);
    assert(items.item_size == sizeof(Expression*));

    ArrayLiteralExpression* array = memory_alloc(sizeof(ArrayLiteralExpression));
    if (!array) { return ALLOCATION_FAILED; }

    *array = (ArrayLiteralExpression){
        .base          = EXPRESSION_INIT(ARRAY_VTABLE, start_token),
        .inferred_size = inferred_size,
        .items         = items,
    };

    *array_expr = array;
    return SUCCESS;
}

void array_literal_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    if (!node) { return; }
    assert(free_alloc);

    ArrayLiteralExpression* array = (ArrayLiteralExpression*)node;
    free_expression_list(&array->items, free_alloc);

    free_alloc(array);
}

NODISCARD Status array_literal_expression_reconstruct(Node*          node,
                                                      const HashMap* symbol_map,
                                                      StringBuilder* sb) {
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

NODISCARD Status array_literal_expression_analyze(Node*            node,
                                                  SemanticContext* parent,
                                                  ArrayList*       errors) {
    ASSERT_EXPRESSION(node);
    assert(parent && errors);

    // Explicitly sized arrays have their argument number enforced by the size
    ArrayLiteralExpression* array = (ArrayLiteralExpression*)node;
    assert(array->items.data && array->items.length > 0);

    const Token     start_token = node->start_token;
    const Allocator allocator   = semantic_context_allocator(parent);

    ArrayListConstIterator it = array_list_const_iterator_init(&array->items);
    Expression*            item;
    UNREACHABLE_IF(!array_list_const_iterator_has_next(&it, (void*)&item));

    TRY(NODE_VIRTUAL_ANALYZE(item, parent, errors));
    SemanticType* inner_type = semantic_context_move_analyzed(parent);
    if (!inner_type->valued) {
        PUT_STATUS_PROPAGATE(errors,
                             NON_VALUED_ARRAY_ITEM,
                             start_token,
                             RC_RELEASE(inner_type, allocator.free_alloc));
    }

    // All following types must be exactly equal to the first type
    while (array_list_const_iterator_has_next(&it, (void*)&item)) {
        TRY_DO(NODE_VIRTUAL_ANALYZE(item, parent, errors),
               RC_RELEASE(inner_type, allocator.free_alloc));
        SemanticType* next_type = semantic_context_move_analyzed(parent);

        if (!type_equal(inner_type, next_type)) {
            PUT_STATUS_PROPAGATE(errors, ARRAY_ITEM_TYPE_MISMATCH, start_token, {
                RC_RELEASE(inner_type, allocator.free_alloc);
                RC_RELEASE(next_type, allocator.free_alloc);
            });
        }

        RC_RELEASE(next_type, allocator.free_alloc);
    }

    SemanticType* resulting_type;
    TRY_DO(semantic_type_create(&resulting_type, allocator.memory_alloc),
           RC_RELEASE(inner_type, allocator.free_alloc));
    resulting_type->tag      = STYPE_ARRAY;
    resulting_type->is_const = false;
    resulting_type->valued   = true;
    resulting_type->nullable = false;

    SemanticArrayType* sarray;
    TRY_DO(semantic_array_create(STYPE_ARRAY_SINGLE_DIM,
                                 (SemanticArrayUnion){.length = array->items.length},
                                 inner_type,
                                 &sarray,
                                 allocator.memory_alloc),
           {
               RC_RELEASE(inner_type, allocator.free_alloc);
               RC_RELEASE(resulting_type, allocator.free_alloc);
           });

    // Everything allocated is moved into the resulting type
    resulting_type->variant = (SemanticTypeUnion){.array_type = sarray};
    parent->analyzed_type   = resulting_type;
    return SUCCESS;
}
