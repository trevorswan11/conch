#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include "lexer/token.h"

#include "ast/expressions/bool.h"

#include "util/allocator.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"
#include "util/mem.h"
#include "util/status.h"

TRY_STATUS bool_literal_expression_create(bool                    value,
                                          BoolLiteralExpression** bool_expr,
                                          memory_alloc_fn         memory_alloc) {
    assert(memory_alloc);
    BoolLiteralExpression* bool_local = memory_alloc(sizeof(BoolLiteralExpression));
    if (!bool_local) {
        return ALLOCATION_FAILED;
    }

    *bool_local = (BoolLiteralExpression){
        .base  = EXPRESSION_INIT(BOOL_VTABLE),
        .value = value,
    };

    *bool_expr = bool_local;
    return SUCCESS;
}

void bool_literal_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    ASSERT_NODE(node);
    assert(free_alloc);
    BoolLiteralExpression* bool_expr = (BoolLiteralExpression*)node;
    free_alloc(bool_expr);
}

Slice bool_literal_expression_token_literal(Node* node) {
    ASSERT_NODE(node);
    BoolLiteralExpression* bool_expr = (BoolLiteralExpression*)node;
    return bool_expr->value ? slice_from_str_z("true") : slice_from_str_z("false");
}

TRY_STATUS
bool_literal_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    ASSERT_NODE(node);
    if (!sb) {
        return NULL_PARAMETER;
    }
    MAYBE_UNUSED(symbol_map);

    PROPAGATE_IF_ERROR(string_builder_append_slice(sb, node->vtable->token_literal(node)));
    return SUCCESS;
}
