#include <assert.h>

#include "ast/ast.h"
#include "ast/expressions/assignment.h"

#include "semantic/context.h"
#include "semantic/symbol.h"
#include "semantic/type.h"

#include "util/containers/string_builder.h"

NODISCARD Status assignment_expression_create(Token                  start_token,
                                              Expression*            lhs,
                                              TokenType              op,
                                              Expression*            rhs,
                                              AssignmentExpression** assignment_expr,
                                              memory_alloc_fn        memory_alloc) {
    assert(memory_alloc);
    assert(lhs && rhs);

    AssignmentExpression* assign = memory_alloc(sizeof(AssignmentExpression));
    if (!assign) {
        return ALLOCATION_FAILED;
    }

    *assign = (AssignmentExpression){
        .base = EXPRESSION_INIT(ASSIGNMENT_VTABLE, start_token),
        .lhs  = lhs,
        .op   = op,
        .rhs  = rhs,
    };

    *assignment_expr = assign;
    return SUCCESS;
}

void assignment_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    if (!node) {
        return;
    }
    assert(free_alloc);

    AssignmentExpression* assign = (AssignmentExpression*)node;
    NODE_VIRTUAL_FREE(assign->lhs, free_alloc);
    NODE_VIRTUAL_FREE(assign->rhs, free_alloc);

    free_alloc(assign);
}

NODISCARD Status assignment_expression_reconstruct(Node*          node,
                                                   const HashMap* symbol_map,
                                                   StringBuilder* sb) {
    ASSERT_EXPRESSION(node);
    assert(sb);

    AssignmentExpression* assign = (AssignmentExpression*)node;
    Slice                 op     = poll_tt_symbol(symbol_map, assign->op);

    if (group_expressions) {
        TRY(string_builder_append(sb, '('));
    }
    ASSERT_EXPRESSION(assign->lhs);
    TRY(NODE_VIRTUAL_RECONSTRUCT(assign->lhs, symbol_map, sb));

    TRY(string_builder_append(sb, ' '));
    TRY(string_builder_append_slice(sb, op));
    TRY(string_builder_append(sb, ' '));

    ASSERT_EXPRESSION(assign->rhs);
    TRY(NODE_VIRTUAL_RECONSTRUCT(assign->rhs, symbol_map, sb));
    if (group_expressions) {
        TRY(string_builder_append(sb, ')'));
    }

    return SUCCESS;
}

NODISCARD Status assignment_expression_analyze(Node*            node,
                                               SemanticContext* parent,
                                               ArrayList*       errors) {
    ASSERT_EXPRESSION(node);
    assert(parent && errors);

    AssignmentExpression* assign = (AssignmentExpression*)node;
    ASSERT_EXPRESSION(assign->lhs);
    ASSERT_EXPRESSION(assign->rhs);

    const Token   start_token = node->start_token;
    free_alloc_fn free_alloc  = parent->symbol_table->symbols.allocator.free_alloc;

    TRY(NODE_VIRTUAL_ANALYZE(assign->lhs, parent, errors));
    SemanticType* lhs_type = semantic_context_move_analyzed(parent);

    if (lhs_type->is_const) {
        IGNORE_STATUS(
            put_status_error(errors, ASSIGNMENT_TO_CONSTANT, start_token.line, start_token.column));

        rc_release(lhs_type, free_alloc);
        return ASSIGNMENT_TO_CONSTANT;
    }

    TRY_DO(NODE_VIRTUAL_ANALYZE(assign->rhs, parent, errors), rc_release(lhs_type, free_alloc));
    SemanticType* rhs_type = semantic_context_move_analyzed(parent);

    if (!type_assignable(lhs_type, rhs_type)) {
        IGNORE_STATUS(
            put_status_error(errors, TYPE_MISMATCH, start_token.line, start_token.column));

        rc_release(lhs_type, free_alloc);
        rc_release(rhs_type, free_alloc);
        return TYPE_MISMATCH;
    }

    // Assignment expressions return the type of their assigned value
    rc_release(lhs_type, free_alloc);
    parent->analyzed_type = rhs_type;
    return SUCCESS;
}
