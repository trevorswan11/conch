#include <assert.h>

#include "ast/expressions/bool.h"

#include "semantic/context.h"
#include "semantic/symbol.h"
#include "semantic/type.h"

#include "util/containers/string_builder.h"

NODISCARD Status bool_literal_expression_create(Token                   start_token,
                                                BoolLiteralExpression** bool_expr,
                                                memory_alloc_fn         memory_alloc) {
    assert(memory_alloc);
    assert(start_token.type == TRUE || start_token.type == FALSE);
    BoolLiteralExpression* bool_local = memory_alloc(sizeof(BoolLiteralExpression));
    if (!bool_local) {
        return ALLOCATION_FAILED;
    }

    *bool_local = (BoolLiteralExpression){
        .base  = EXPRESSION_INIT(BOOL_VTABLE, start_token),
        .value = start_token.type == TRUE,
    };

    *bool_expr = bool_local;
    return SUCCESS;
}

void bool_literal_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    if (!node) {
        return;
    }
    assert(free_alloc);

    BoolLiteralExpression* bool_expr = (BoolLiteralExpression*)node;
    free_alloc(bool_expr);
}

NODISCARD Status bool_literal_expression_reconstruct(Node*          node,
                                                     const HashMap* symbol_map,
                                                     StringBuilder* sb) {
    ASSERT_EXPRESSION(node);
    MAYBE_UNUSED(symbol_map);
    assert(sb);

    BoolLiteralExpression* bool_expr = (BoolLiteralExpression*)node;
    TRY(string_builder_append_str_z(sb, bool_expr->value ? "true" : "false"));
    return SUCCESS;
}

NODISCARD Status bool_literal_expression_analyze(Node*            node,
                                                 SemanticContext* parent,
                                                 ArrayList*       errors) {
    PRIMITIVE_ANALYZE(STYPE_BOOL, false, parent->symbol_table->symbols.allocator.memory_alloc);
}
