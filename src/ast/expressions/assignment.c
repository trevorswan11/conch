#include <assert.h>

#include "ast/ast.h"
#include "ast/expressions/assignment.h"

#include "semantic/context.h"

#include "util/containers/array_list.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"

#define ASSIGNMENT_FREE(type)                  \
    ASSERT_NODE(node);                         \
    assert(free_alloc);                        \
                                               \
    type* infix = (type*)node;                 \
    NODE_VIRTUAL_FREE(infix->lhs, free_alloc); \
    NODE_VIRTUAL_FREE(infix->rhs, free_alloc); \
                                               \
    free_alloc(infix)

NODISCARD Status assignment_expression_create(Token                  start_token,
                                              Expression*            lhs,
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
        .rhs  = rhs,
    };

    *assignment_expr = assign;
    return SUCCESS;
}

void assignment_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    ASSIGNMENT_FREE(AssignmentExpression);
}

NODISCARD Status assignment_expression_reconstruct(Node*          node,
                                                   const HashMap* symbol_map,
                                                   StringBuilder* sb) {
    ASSERT_NODE(node);
    assert(sb);

    AssignmentExpression* assign = (AssignmentExpression*)node;
    assert(assign->lhs && assign->rhs);

    Node* lhs = (Node*)assign->lhs;
    Node* rhs = (Node*)assign->rhs;

    if (group_expressions) {
        TRY(string_builder_append(sb, '('));
    }
    TRY(lhs->vtable->reconstruct(lhs, symbol_map, sb));

    TRY(string_builder_append_str_z(sb, " = "));

    TRY(rhs->vtable->reconstruct(rhs, symbol_map, sb));
    if (group_expressions) {
        TRY(string_builder_append(sb, ')'));
    }

    return SUCCESS;
}

NODISCARD Status assignment_expression_analyze(Node*            node,
                                               SemanticContext* parent,
                                               ArrayList*       errors) {
    assert(node && parent && errors);
    MAYBE_UNUSED(node);
    MAYBE_UNUSED(parent);
    MAYBE_UNUSED(errors);
    return NOT_IMPLEMENTED;
}

NODISCARD Status compound_assignment_expression_create(Token                          start_token,
                                                       Expression*                    lhs,
                                                       TokenType                      op,
                                                       Expression*                    rhs,
                                                       CompoundAssignmentExpression** compound_expr,
                                                       memory_alloc_fn memory_alloc) {
    assert(memory_alloc);
    assert(lhs && rhs);

    CompoundAssignmentExpression* compound = memory_alloc(sizeof(CompoundAssignmentExpression));
    if (!compound) {
        return ALLOCATION_FAILED;
    }

    *compound = (CompoundAssignmentExpression){
        .base = EXPRESSION_INIT(COMPOUND_ASSIGNMENT_VTABLE, start_token),
        .lhs  = lhs,
        .op   = op,
        .rhs  = rhs,
    };

    *compound_expr = compound;
    return SUCCESS;
}

void compound_assignment_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    ASSIGNMENT_FREE(CompoundAssignmentExpression);
}

NODISCARD Status compound_assignment_expression_reconstruct(Node*          node,
                                                            const HashMap* symbol_map,
                                                            StringBuilder* sb) {
    ASSERT_NODE(node);
    assert(sb);

    CompoundAssignmentExpression* compound = (CompoundAssignmentExpression*)node;
    assert(compound->lhs && compound->rhs);

    Node* lhs = (Node*)compound->lhs;
    Slice op  = poll_tt_symbol(symbol_map, compound->op);
    Node* rhs = (Node*)compound->rhs;

    if (group_expressions) {
        TRY(string_builder_append(sb, '('));
    }
    TRY(lhs->vtable->reconstruct(lhs, symbol_map, sb));

    TRY(string_builder_append(sb, ' '));
    TRY(string_builder_append_slice(sb, op));
    TRY(string_builder_append(sb, ' '));

    TRY(rhs->vtable->reconstruct(rhs, symbol_map, sb));
    if (group_expressions) {
        TRY(string_builder_append(sb, ')'));
    }

    return SUCCESS;
}

NODISCARD Status compound_assignment_expression_analyze(Node*            node,
                                                        SemanticContext* parent,
                                                        ArrayList*       errors) {
    assert(node && parent && errors);
    MAYBE_UNUSED(node);
    MAYBE_UNUSED(parent);
    MAYBE_UNUSED(errors);
    return NOT_IMPLEMENTED;
}
