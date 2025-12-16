#include <assert.h>

#include "ast/expressions/enum.h"

#include "semantic/context.h"

#include "util/containers/string_builder.h"

void free_enum_variant_list(ArrayList* variants, free_alloc_fn free_alloc) {
    assert(variants);
    assert(free_alloc);

    ArrayListIterator it = array_list_iterator_init(variants);
    EnumVariant       variant;
    while (array_list_iterator_has_next(&it, &variant)) {
        NODE_VIRTUAL_FREE(variant.name, free_alloc);
        NODE_VIRTUAL_FREE(variant.value, free_alloc);
    }

    array_list_deinit(variants);
}

NODISCARD Status enum_expression_create(Token            start_token,
                                        ArrayList        variants,
                                        EnumExpression** enum_expr,
                                        memory_alloc_fn  memory_alloc) {
    assert(variants.item_size == sizeof(EnumVariant));
    assert(variants.length > 0);
    assert(memory_alloc);

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
    if (!node) {
        return;
    }
    assert(free_alloc);

    EnumExpression* enum_expr = (EnumExpression*)node;
    free_enum_variant_list(&enum_expr->variants, free_alloc);

    free_alloc(enum_expr);
}

NODISCARD Status enum_expression_reconstruct(Node*          node,
                                             const HashMap* symbol_map,
                                             StringBuilder* sb) {
    ASSERT_EXPRESSION(node);
    assert(sb);

    EnumExpression* enum_expr = (EnumExpression*)node;
    TRY(string_builder_append_str_z(sb, "enum { "));

    ArrayListIterator it = array_list_iterator_init(&enum_expr->variants);
    EnumVariant       variant;
    while (array_list_iterator_has_next(&it, &variant)) {
        ASSERT_EXPRESSION(variant.name);
        TRY(NODE_VIRTUAL_RECONSTRUCT(variant.name, symbol_map, sb));

        if (variant.value) {
            ASSERT_EXPRESSION(variant.value);
            TRY(string_builder_append_str_z(sb, " = "));
            TRY(NODE_VIRTUAL_RECONSTRUCT(variant.value, symbol_map, sb));
        }

        TRY(string_builder_append_str_z(sb, ", "));
    }

    TRY(string_builder_append(sb, '}'));
    return SUCCESS;
}

NODISCARD Status enum_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors) {
    ASSERT_EXPRESSION(node);
    assert(parent && errors);

    EnumExpression* enum_expr = (EnumExpression*)node;
    assert(enum_expr->variants.data && enum_expr->variants.length > 0);

    MAYBE_UNUSED(enum_expr);
    MAYBE_UNUSED(parent);
    MAYBE_UNUSED(errors);
    return NOT_IMPLEMENTED;
}
