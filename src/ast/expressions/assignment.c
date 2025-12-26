#include <assert.h>

#include "ast/ast.h"
#include "ast/expressions/assignment.h"
#include "ast/expressions/infix.h"

#include "semantic/context.h"
#include "semantic/type.h"

#include "util/containers/string_builder.h"

[[nodiscard]] Status assignment_expression_create(Token                  start_token,
                                              Expression*            lhs,
                                              TokenType              op,
                                              Expression*            rhs,
                                              AssignmentExpression** assignment_expr,
                                              memory_alloc_fn        memory_alloc) {
    assert(memory_alloc);
    ASSERT_EXPRESSION(lhs);
    ASSERT_EXPRESSION(rhs);

    AssignmentExpression* assign = memory_alloc(sizeof(AssignmentExpression));
    if (!assign) { return ALLOCATION_FAILED; }

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
    if (!node) { return; }
    assert(free_alloc);

    AssignmentExpression* assign = (AssignmentExpression*)node;
    NODE_VIRTUAL_FREE(assign->lhs, free_alloc);
    NODE_VIRTUAL_FREE(assign->rhs, free_alloc);

    free_alloc(assign);
}

[[nodiscard]] Status assignment_expression_reconstruct(Node*          node,
                                                   const HashMap* symbol_map,
                                                   StringBuilder* sb) {
    ASSERT_EXPRESSION(node);
    assert(sb);

    AssignmentExpression* assign = (AssignmentExpression*)node;
    Slice                 op     = poll_tt_symbol(symbol_map, assign->op);

    if (group_expressions) { TRY(string_builder_append(sb, '(')); }
    ASSERT_EXPRESSION(assign->lhs);
    TRY(NODE_VIRTUAL_RECONSTRUCT(assign->lhs, symbol_map, sb));

    TRY(string_builder_append(sb, ' '));
    TRY(string_builder_append_slice(sb, op));
    TRY(string_builder_append(sb, ' '));

    ASSERT_EXPRESSION(assign->rhs);
    TRY(NODE_VIRTUAL_RECONSTRUCT(assign->rhs, symbol_map, sb));
    if (group_expressions) { TRY(string_builder_append(sb, ')')); }

    return SUCCESS;
}

[[nodiscard]] Status assignment_expression_analyze(Node*            node,
                                               SemanticContext* parent,
                                               ArrayList*       errors) {
    ASSERT_EXPRESSION(node);
    assert(parent && errors);

    AssignmentExpression* assign = (AssignmentExpression*)node;
    ASSERT_EXPRESSION(assign->lhs);
    ASSERT_EXPRESSION(assign->rhs);

    const Token     start_token = node->start_token;
    const Allocator allocator   = semantic_context_allocator(parent);

    TRY(NODE_VIRTUAL_ANALYZE(assign->lhs, parent, errors));
    SemanticType* lhs_type = semantic_context_move_analyzed(parent);

    if (lhs_type->is_const) {
        PUT_STATUS_PROPAGATE(errors,
                             ASSIGNMENT_TO_CONSTANT,
                             start_token,
                             RC_RELEASE(lhs_type, allocator.free_alloc));
    }

    TRY_DO(NODE_VIRTUAL_ANALYZE(assign->rhs, parent, errors),
           RC_RELEASE(lhs_type, allocator.free_alloc));
    SemanticType* rhs_type = semantic_context_move_analyzed(parent);

    if (assign->op != ASSIGN) {
        // Sometimes a non valued type is valid for regular assignment
        if (!lhs_type->valued || !rhs_type->valued) {
            const Status error_code =
                !lhs_type->valued ? ILLEGAL_LHS_INFIX_OPERAND : ILLEGAL_RHS_INFIX_OPERAND;
            PUT_STATUS_PROPAGATE(errors, error_code, start_token, {
                RC_RELEASE(lhs_type, allocator.free_alloc);
                RC_RELEASE(rhs_type, allocator.free_alloc);
            });
        }

        // Neither side can be nullable here since nil is explicit
        if (lhs_type->nullable || rhs_type->nullable) {
            const Status error_code =
                lhs_type->nullable ? ILLEGAL_LHS_INFIX_OPERAND : ILLEGAL_RHS_INFIX_OPERAND;
            PUT_STATUS_PROPAGATE(errors, error_code, start_token, {
                RC_RELEASE(lhs_type, allocator.free_alloc);
                RC_RELEASE(rhs_type, allocator.free_alloc);
            });
        }
    }

    // Compound expressions are treated like a subset of infix expressions
    SemanticType* resulting_type = nullptr;
    switch (assign->op) {
    case ASSIGN:
        if (!type_assignable(lhs_type, rhs_type)) {
            PUT_STATUS_PROPAGATE(errors, TYPE_MISMATCH, start_token, {
                RC_RELEASE(lhs_type, allocator.free_alloc);
                RC_RELEASE(rhs_type, allocator.free_alloc);
            });
        }
        resulting_type = rc_retain(rhs_type);
        break;
    case PLUS_ASSIGN:
    case STAR_ASSIGN: {
        PLUS_STAR_INFIX_CASE
    }
    case AND_ASSIGN:
    case OR_ASSIGN:
    case XOR_ASSIGN:
    case SHR_ASSIGN:
    case SHL_ASSIGN:
    case PERCENT_ASSIGN: {
        MODULE_BITWISE_INFIX_CASE
    }
    case MINUS_ASSIGN:
    case SLASH_ASSIGN: {
        FALLBACK_ARITHMETIC(resulting_type, {
            RC_RELEASE(lhs_type, allocator.free_alloc);
            RC_RELEASE(rhs_type, allocator.free_alloc);
        });
        break;
    }
    case NOT_ASSIGN: {
        const bool lhs_ok = semantic_type_is_integer(lhs_type);
        const bool rhs_ok = semantic_type_is_integer(rhs_type);
        if (!lhs_ok || !rhs_ok) {
            PUT_STATUS_PROPAGATE(errors,
                                 !lhs_ok ? ILLEGAL_LHS_INFIX_OPERAND : ILLEGAL_RHS_INFIX_OPERAND,
                                 start_token,
                                 {
                                     RC_RELEASE(lhs_type, allocator.free_alloc);
                                     RC_RELEASE(rhs_type, allocator.free_alloc);
                                 });
        }

        if (lhs_type->tag != rhs_type->tag) {
            PUT_STATUS_PROPAGATE(errors, TYPE_MISMATCH, start_token, {
                RC_RELEASE(lhs_type, allocator.free_alloc);
                RC_RELEASE(rhs_type, allocator.free_alloc);
            });
        }

        MAKE_PRIMITIVE(rhs_type->tag, false, new_tye, allocator.memory_alloc, {
            RC_RELEASE(lhs_type, allocator.free_alloc);
            RC_RELEASE(rhs_type, allocator.free_alloc);
        });

        resulting_type = new_tye;
        break;
    }
    default:
        UNREACHABLE;
    }

    // Assignment expressions return the type of their assigned value
    RC_RELEASE(lhs_type, allocator.free_alloc);
    RC_RELEASE(rhs_type, allocator.free_alloc);

    assert(resulting_type);
    parent->analyzed_type = resulting_type;
    return SUCCESS;
}
