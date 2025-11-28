#include <assert.h>

#include "ast/expressions/enum.h"

TRY_STATUS enum_expression_create(Token            start_token,
                                  ArrayList        variants,
                                  EnumExpression** enum_expr,
                                  memory_alloc_fn  memory_alloc) {
    assert(variants.item_size == sizeof(EnumVariant));
    assert(memory_alloc);

    if (variants.length == 0) {
        return ENUM_MISSING_VARIANTS;
    }

    EnumExpression* enum_local = memory_alloc(sizeof(EnumExpression));
    if (!enum_local) {
        return ALLOCATION_FAILED;
    }

    *enum_local = (EnumExpression){
        .base     = EXPRESSION_INIT(ENUM_VTABLE, start_token),
        .variants = variants,
    };

    *enum_expr = enum_local;
    return SUCCESS;
}

void enum_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    ASSERT_NODE(node);
    assert(free_alloc);

    EnumExpression* e = (EnumExpression*)node;
    EnumVariant     variant;
    for (size_t i = 0; i < e->variants.length; i++) {
        UNREACHABLE_IF_ERROR(array_list_get(&e->variants, i, &variant));
        identifier_expression_destroy((Node*)variant.name, free_alloc);
    }
}

TRY_STATUS
enum_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    ASSERT_NODE(node);
    if (!sb) {
        return NULL_PARAMETER;
    }

    EnumExpression* e = (EnumExpression*)node;
    PROPAGATE_IF_ERROR(string_builder_append_many(sb, "enum { ", 7));

    EnumVariant variant;
    for (size_t i = 0; i < e->variants.length; i++) {
        UNREACHABLE_IF_ERROR(array_list_get(&e->variants, i, &variant));

        PROPAGATE_IF_ERROR(identifier_expression_reconstruct((Node*)variant.name, symbol_map, sb));
        PROPAGATE_IF_ERROR(string_builder_append_many(sb, " = ", 3));
        PROPAGATE_IF_ERROR(string_builder_append_signed(sb, variant.value));

        PROPAGATE_IF_ERROR(string_builder_append_many(sb, ", ", 2));
    }
    PROPAGATE_IF_ERROR(string_builder_append_many(sb, "};", 2));

    return NOT_IMPLEMENTED;
}
