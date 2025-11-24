#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include "lexer/token.h"

#include "ast/ast.h"
#include "ast/expressions/infix.h"

#include "util/allocator.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"
#include "util/mem.h"
#include "util/status.h"

TRY_STATUS infix_expression_create(Expression*       lhs,
                                   TokenType         op,
                                   Expression*       rhs,
                                   InfixExpression** infix_expr,
                                   memory_alloc_fn   memory_alloc) {
    assert(memory_alloc);
    if (!lhs || !rhs) {
        return NULL_PARAMETER;
    }

    InfixExpression* infix = memory_alloc(sizeof(InfixExpression));
    if (!infix) {
        return ALLOCATION_FAILED;
    }

    *infix = (InfixExpression){
        .base = EXPRESSION_INIT(INFIX_VTABLE),
        .lhs  = lhs,
        .op   = op,
        .rhs  = rhs,
    };

    *infix_expr = infix;
    return SUCCESS;
}

void infix_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    ASSERT_NODE(node);
    assert(free_alloc);
    InfixExpression* infix = (InfixExpression*)node;

    NODE_VIRTUAL_FREE(infix->lhs, free_alloc);
    infix->lhs = NULL;

    NODE_VIRTUAL_FREE(infix->rhs, free_alloc);
    infix->rhs = NULL;

    free_alloc(infix);
}

Slice infix_expression_token_literal(Node* node) {
    ASSERT_NODE(node);
    InfixExpression* infix = (InfixExpression*)node;
    return slice_from_str_z(token_type_name(infix->op));
}

TRY_STATUS infix_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    ASSERT_NODE(node);
    if (!sb) {
        return NULL_PARAMETER;
    }

    InfixExpression* infix = (InfixExpression*)node;
    assert(infix->lhs && infix->rhs);

    Node* lhs = (Node*)infix->lhs;
    Slice op  = poll_tt_symbol(symbol_map, infix->op);
    Node* rhs = (Node*)infix->rhs;

    PROPAGATE_IF_ERROR(string_builder_append(sb, '('));
    PROPAGATE_IF_ERROR(lhs->vtable->reconstruct(lhs, symbol_map, sb));

    PROPAGATE_IF_ERROR(string_builder_append(sb, ' '));
    PROPAGATE_IF_ERROR(string_builder_append_slice(sb, op));
    PROPAGATE_IF_ERROR(string_builder_append(sb, ' '));

    PROPAGATE_IF_ERROR(rhs->vtable->reconstruct(rhs, symbol_map, sb));
    PROPAGATE_IF_ERROR(string_builder_append(sb, ')'));

    return SUCCESS;
}
