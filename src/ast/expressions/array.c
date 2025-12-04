#include <assert.h>

#include "ast/ast.h"
#include "ast/expressions/array.h"

TRY_STATUS array_literal_expression_create(Token                    start_token,
                                           bool                     inferred_size,
                                           ArrayList                items,
                                           ArrayLiteralExpression** array_expr,
                                           memory_alloc_fn          memory_alloc) {
    assert(memory_alloc);
    assert(items.item_size == sizeof(Expression*));

    ArrayLiteralExpression* array = memory_alloc(sizeof(ArrayLiteralExpression));
    if (!array) {
        return ALLOCATION_FAILED;
    }

    *array = (ArrayLiteralExpression){
        .base          = EXPRESSION_INIT(ARRAY_VTABLE, start_token),
        .inferred_size = inferred_size,
        .items         = items,
    };

    *array_expr = array;
    return SUCCESS;
}

void array_literal_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    ASSERT_NODE(node);
    assert(free_alloc);

    ArrayLiteralExpression* array = (ArrayLiteralExpression*)node;
    free_expression_list(&array->items, free_alloc);

    free_alloc(array);
}

TRY_STATUS
array_literal_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    ASSERT_NODE(node);
    if (!sb) {
        return NULL_PARAMETER;
    }

    ArrayLiteralExpression* array = (ArrayLiteralExpression*)node;
    PROPAGATE_IF_ERROR(string_builder_append(sb, '['));
    if (array->inferred_size) {
        PROPAGATE_IF_ERROR(string_builder_append(sb, '_'));
    } else {
        PROPAGATE_IF_ERROR(string_builder_append_unsigned(sb, (uint64_t)array->items.length));
        PROPAGATE_IF_ERROR(string_builder_append(sb, 'u'));
    }
    PROPAGATE_IF_ERROR(string_builder_append_str_z(sb, "]{ "));

    Expression* item;
    for (size_t i = 0; i < array->items.length; i++) {
        UNREACHABLE_IF_ERROR(array_list_get(&array->items, i, &item));

        Node* item_node = (Node*)item;
        PROPAGATE_IF_ERROR(item_node->vtable->reconstruct(item_node, symbol_map, sb));
        PROPAGATE_IF_ERROR(string_builder_append_str_z(sb, ", "));
    }

    PROPAGATE_IF_ERROR(string_builder_append(sb, '}'));
    return SUCCESS;
}
