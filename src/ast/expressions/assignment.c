#include <assert.h>

#include "ast/ast.h"
#include "ast/expressions/assignment.h"

#include "semantic/context.h"
#include "semantic/type.h"

#include "util/containers/string_builder.h"

NODISCARD Status assignment_expression_create(Token                  start_token,
                                              Expression*            lhs,
                                              TokenType              op,
                                              Expression*            rhs,
                                              AssignmentExpression** assignment_expr,
                                              memory_alloc_fn        memory_alloc) {
    assert(memory_alloc);
    ASSERT_EXPRESSION(lhs);
    ASSERT_EXPRESSION(rhs);

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
    free_alloc_fn free_alloc  = semantic_context_allocator(parent).free_alloc;

    TRY(NODE_VIRTUAL_ANALYZE(assign->lhs, parent, errors));
    SemanticType* lhs_type = semantic_context_move_analyzed(parent);

    if (lhs_type->is_const) {
        PUT_STATUS_PROPAGATE(
            errors, ASSIGNMENT_TO_CONSTANT, start_token, RC_RELEASE(lhs_type, free_alloc));
    }

    TRY_DO(NODE_VIRTUAL_ANALYZE(assign->rhs, parent, errors), RC_RELEASE(lhs_type, free_alloc));
    SemanticType* rhs_type = semantic_context_move_analyzed(parent);

    // Compound expressions are treated like a subset of infix expressions
    // TODO: Narrow in restrictions as in infix
    bool ok = type_assignable(lhs_type, rhs_type);
    if (assign->op != ASSIGN) {
        // Neither side can be nullable and must be primitive
        ok = ok && !lhs_type->nullable && !rhs_type->nullable;
        ok = ok && semantic_type_is_primitive(lhs_type) && semantic_type_is_primitive(rhs_type);
    }

    if (!ok) {
        PUT_STATUS_PROPAGATE(errors, TYPE_MISMATCH, start_token, {
            RC_RELEASE(lhs_type, free_alloc);
            RC_RELEASE(rhs_type, free_alloc);
        });
    }

    // Assignment expressions return the type of their assigned value
    RC_RELEASE(lhs_type, free_alloc);
    parent->analyzed_type = rhs_type;
    return SUCCESS;
}
