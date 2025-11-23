#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include "lexer/token.h"

#include "ast/expressions/string.h"

#include "util/allocator.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"
#include "util/mem.h"
#include "util/status.h"

TRY_STATUS string_literal_expression_create(Token                     token,
                                            StringLiteralExpression** string_expr,
                                            Allocator                 allocator) {
    ASSERT_ALLOCATOR(allocator);
    StringLiteralExpression* string = allocator.memory_alloc(sizeof(StringLiteralExpression));
    if (!string) {
        return ALLOCATION_FAILED;
    }

    MutSlice slice;
    PROPAGATE_IF_ERROR_DO(promote_token_string(token, &slice, allocator),
                          allocator.free_alloc(string));

    *string = (StringLiteralExpression){
        .base  = EXPRESSION_INIT(STRING_VTABLE),
        .token = token,
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

Slice string_literal_expression_token_literal(Node* node) {
    ASSERT_NODE(node);
    StringLiteralExpression* string_expr = (StringLiteralExpression*)node;
    return slice_from_mut(&string_expr->slice);
}

TRY_STATUS
string_literal_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    ASSERT_NODE(node);
    if (!sb) {
        return NULL_PARAMETER;
    }
    MAYBE_UNUSED(symbol_map);

    StringLiteralExpression* string_expr = (StringLiteralExpression*)node;
    PROPAGATE_IF_ERROR(string_builder_append_slice(sb, string_expr->token.slice));
    return SUCCESS;
}
