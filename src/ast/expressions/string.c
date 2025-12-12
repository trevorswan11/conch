#include <assert.h>

#include "ast/expressions/string.h"

#include "semantic/context.h"

#include "util/containers/array_list.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"

NODISCARD Status string_literal_expression_create(Token                     start_token,
                                                  StringLiteralExpression** string_expr,
                                                  Allocator                 allocator) {
    ASSERT_ALLOCATOR(allocator);
    StringLiteralExpression* string = allocator.memory_alloc(sizeof(StringLiteralExpression));
    if (!string) {
        return ALLOCATION_FAILED;
    }

    MutSlice slice;
    TRY_DO(promote_token_string(start_token, &slice, allocator), allocator.free_alloc(string));

    *string = (StringLiteralExpression){
        .base  = EXPRESSION_INIT(STRING_VTABLE, start_token),
        .slice = slice,
    };

    *string_expr = string;
    return SUCCESS;
}

void string_literal_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    ASSERT_NODE(node);
    assert(free_alloc);

    StringLiteralExpression* string_expr = (StringLiteralExpression*)node;
    if (string_expr->slice.ptr) {
        free_alloc(string_expr->slice.ptr);
    }

    free_alloc(string_expr);
}

NODISCARD Status string_literal_expression_reconstruct(Node*          node,
                                                       const HashMap* symbol_map,
                                                       StringBuilder* sb) {
    ASSERT_NODE(node);
    assert(sb);
    MAYBE_UNUSED(symbol_map);

    // The tokenizer drops the start of multiline strings so we have to reconstruct here
    if (node->start_token.type == MULTILINE_STRING) {
        TRY(string_builder_append_str_z(sb, "\\\\"));
    }

    TRY(string_builder_append_slice(sb, node->start_token.slice));
    if (node->start_token.type == MULTILINE_STRING) {
        TRY(string_builder_append(sb, '\n'));
    }
    return SUCCESS;
}

NODISCARD Status string_literal_expression_analyze(Node*            node,
                                                   SemanticContext* parent,
                                                   ArrayList*       errors) {
    assert(node && parent && errors);
    MAYBE_UNUSED(node);
    MAYBE_UNUSED(parent);
    MAYBE_UNUSED(errors);
    return NOT_IMPLEMENTED;
}
