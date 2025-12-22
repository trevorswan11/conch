#include <assert.h>

#include "ast/ast.h"
#include "ast/expressions/infix.h"

#include "semantic/context.h"
#include "semantic/type.h"

#include "util/containers/string_builder.h"

NODISCARD Status infix_expression_create(Token             start_token,
                                         Expression*       lhs,
                                         TokenType         op,
                                         Expression*       rhs,
                                         InfixExpression** infix_expr,
                                         memory_alloc_fn   memory_alloc) {
    assert(memory_alloc);
    assert(lhs && rhs);

    InfixExpression* infix = memory_alloc(sizeof(InfixExpression));
    if (!infix) {
        return ALLOCATION_FAILED;
    }

    *infix = (InfixExpression){
        .base = EXPRESSION_INIT(INFIX_VTABLE, start_token),
        .lhs  = lhs,
        .op   = op,
        .rhs  = rhs,
    };

    *infix_expr = infix;
    return SUCCESS;
}

void infix_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    if (!node) {
        return;
    }
    assert(free_alloc);

    InfixExpression* infix = (InfixExpression*)node;
    NODE_VIRTUAL_FREE(infix->lhs, free_alloc);
    NODE_VIRTUAL_FREE(infix->rhs, free_alloc);

    free_alloc(infix);
}

NODISCARD Status infix_expression_reconstruct(Node*          node,
                                              const HashMap* symbol_map,
                                              StringBuilder* sb) {
    ASSERT_EXPRESSION(node);
    assert(sb);

    InfixExpression* infix = (InfixExpression*)node;
    Slice            op    = poll_tt_symbol(symbol_map, infix->op);

    if (group_expressions) {
        TRY(string_builder_append(sb, '('));
    }
    ASSERT_EXPRESSION(infix->lhs);
    TRY(NODE_VIRTUAL_RECONSTRUCT(infix->lhs, symbol_map, sb));

    TRY(string_builder_append(sb, ' '));
    TRY(string_builder_append_slice(sb, op));
    TRY(string_builder_append(sb, ' '));

    ASSERT_EXPRESSION(infix->rhs);
    TRY(NODE_VIRTUAL_RECONSTRUCT(infix->rhs, symbol_map, sb));
    if (group_expressions) {
        TRY(string_builder_append(sb, ')'));
    }

    return SUCCESS;
}

#define FALLBACK_ARITHMETIC(result, cleanup)                                                  \
    const bool lhs_ok = semantic_type_is_arithmetic(lhs_type);                                \
    const bool rhs_ok = semantic_type_is_arithmetic(rhs_type);                                \
    if (!lhs_ok || !rhs_ok) {                                                                 \
        PUT_STATUS_PROPAGATE(errors,                                                          \
                             !lhs_ok ? ILLEGAL_LHS_INFIX_OPERAND : ILLEGAL_RHS_INFIX_OPERAND, \
                             start_token,                                                     \
                             cleanup);                                                        \
    }                                                                                         \
                                                                                              \
    if (lhs_type->tag != rhs_type->tag) {                                                     \
        PUT_STATUS_PROPAGATE(errors, TYPE_MISMATCH, start_token, cleanup);                    \
    }                                                                                         \
                                                                                              \
    MAKE_PRIMITIVE(lhs_type->tag, false, new_type, allocator.memory_alloc, cleanup);          \
    result = new_type;

NODISCARD Status infix_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors) {
    ASSERT_EXPRESSION(node);
    assert(parent && errors);

    const Token     start_token = node->start_token;
    const Allocator allocator   = semantic_context_allocator(parent);

    InfixExpression* infix = (InfixExpression*)node;
    ASSERT_EXPRESSION(infix->lhs);
    ASSERT_EXPRESSION(infix->rhs);

    TRY(NODE_VIRTUAL_ANALYZE(infix->lhs, parent, errors));
    SemanticType* lhs_type = semantic_context_move_analyzed(parent);
    TRY_DO(NODE_VIRTUAL_ANALYZE(infix->rhs, parent, errors),
           RC_RELEASE(lhs_type, allocator.free_alloc));
    SemanticType* rhs_type = semantic_context_move_analyzed(parent);

    if (!lhs_type->valued || !rhs_type->valued) {
        const Status error_code =
            !lhs_type->valued ? ILLEGAL_LHS_INFIX_OPERAND : ILLEGAL_RHS_INFIX_OPERAND;
        PUT_STATUS_PROPAGATE(errors, error_code, start_token, {
            RC_RELEASE(lhs_type, allocator.free_alloc);
            RC_RELEASE(rhs_type, allocator.free_alloc);
        });
    }

    // Subsets of operators have different behaviors and restrictions
    SemanticType* resulting_type = NULL;
    switch (infix->op) {
    case PLUS:
    case STAR: {
        if (lhs_type->tag == STYPE_STR && semantic_type_is_primitive(rhs_type)) {
            if (lhs_type->nullable || rhs_type->nullable) {
                const Status error_code =
                    lhs_type->nullable ? ILLEGAL_LHS_INFIX_OPERAND : ILLEGAL_RHS_INFIX_OPERAND;
                PUT_STATUS_PROPAGATE(errors, error_code, start_token, {
                    RC_RELEASE(lhs_type, allocator.free_alloc);
                    RC_RELEASE(rhs_type, allocator.free_alloc);
                });
            }

            MAKE_PRIMITIVE(STYPE_STR, false, new_type, allocator.memory_alloc, {
                RC_RELEASE(lhs_type, allocator.free_alloc);
                RC_RELEASE(rhs_type, allocator.free_alloc);
            });
            resulting_type = new_type;
            break;
        } else if (semantic_type_is_primitive(lhs_type) && rhs_type->tag == STYPE_STR) {
            PUT_STATUS_PROPAGATE(errors, ILLEGAL_LHS_INFIX_OPERAND, start_token, {
                RC_RELEASE(lhs_type, allocator.free_alloc);
                RC_RELEASE(rhs_type, allocator.free_alloc);
            });
        }

        FALLBACK_ARITHMETIC(resulting_type, {
            RC_RELEASE(lhs_type, allocator.free_alloc);
            RC_RELEASE(rhs_type, allocator.free_alloc);
        });
        break;
    }
    case AND:
    case OR:
    case XOR:
    case SHR:
    case SHL:
    case PERCENT: {
        // Modulo and bitwise cannot support floating points, which the fallback allows
        if (lhs_type->tag == STYPE_FLOATING_POINT || rhs_type->tag == STYPE_FLOATING_POINT) {
            const Status error_code = lhs_type->tag == STYPE_FLOATING_POINT
                                          ? ILLEGAL_LHS_INFIX_OPERAND
                                          : ILLEGAL_RHS_INFIX_OPERAND;
            PUT_STATUS_PROPAGATE(errors, error_code, start_token, {
                RC_RELEASE(lhs_type, allocator.free_alloc);
                RC_RELEASE(rhs_type, allocator.free_alloc);
            });
        }

        FALLBACK_ARITHMETIC(resulting_type, {
            RC_RELEASE(lhs_type, allocator.free_alloc);
            RC_RELEASE(rhs_type, allocator.free_alloc);
        });
        break;
    }
    case MINUS:
    case SLASH:
    case STAR_STAR: {
        FALLBACK_ARITHMETIC(resulting_type, {
            RC_RELEASE(lhs_type, allocator.free_alloc);
            RC_RELEASE(rhs_type, allocator.free_alloc);
        });
        break;
    }
    case LT:
    case LTEQ:
    case GT:
    case GTEQ: {
        const bool lhs_ok =
            semantic_type_is_arithmetic(lhs_type) || lhs_type->tag == STYPE_BYTE_INTEGER;
        const bool rhs_ok =
            semantic_type_is_arithmetic(rhs_type) || rhs_type->tag == STYPE_BYTE_INTEGER;
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

        MAKE_PRIMITIVE(STYPE_BOOL, false, new_type, allocator.memory_alloc, {
            RC_RELEASE(lhs_type, allocator.free_alloc);
            RC_RELEASE(rhs_type, allocator.free_alloc);
        });
        resulting_type = new_type;
        break;
    }
    case EQ:
    case NEQ: {
        const bool lhs_ok = lhs_type->tag == STYPE_NIL || semantic_type_is_primitive(lhs_type);
        const bool rhs_ok = rhs_type->tag == STYPE_NIL || semantic_type_is_primitive(rhs_type);
        if (!lhs_ok || !rhs_ok) {
            PUT_STATUS_PROPAGATE(errors,
                                 !lhs_ok ? ILLEGAL_LHS_INFIX_OPERAND : ILLEGAL_RHS_INFIX_OPERAND,
                                 start_token,
                                 {
                                     RC_RELEASE(lhs_type, allocator.free_alloc);
                                     RC_RELEASE(rhs_type, allocator.free_alloc);
                                 });
        }

        if (lhs_type->tag != rhs_type->tag &&
            (lhs_type->tag != STYPE_NIL && rhs_type->tag != STYPE_NIL)) {
            PUT_STATUS_PROPAGATE(errors, TYPE_MISMATCH, start_token, {
                RC_RELEASE(lhs_type, allocator.free_alloc);
                RC_RELEASE(rhs_type, allocator.free_alloc);
            });
        }

        MAKE_PRIMITIVE(STYPE_BOOL, false, new_type, allocator.memory_alloc, {
            RC_RELEASE(lhs_type, allocator.free_alloc);
            RC_RELEASE(rhs_type, allocator.free_alloc);
        });
        resulting_type = new_type;
        break;
    }
    case BOOLEAN_AND:
    case BOOLEAN_OR: {
        if ((!lhs_type->nullable && !rhs_type->nullable) &&
            (lhs_type->tag != STYPE_BOOL || rhs_type->tag != STYPE_BOOL)) {
            const Status error_code =
                lhs_type->tag != STYPE_BOOL ? ILLEGAL_LHS_INFIX_OPERAND : ILLEGAL_RHS_INFIX_OPERAND;
            PUT_STATUS_PROPAGATE(errors, error_code, start_token, {
                RC_RELEASE(lhs_type, allocator.free_alloc);
                RC_RELEASE(rhs_type, allocator.free_alloc);
            });
        }

        MAKE_PRIMITIVE(STYPE_BOOL, false, new_type, allocator.memory_alloc, {
            RC_RELEASE(lhs_type, allocator.free_alloc);
            RC_RELEASE(rhs_type, allocator.free_alloc);
        });
        resulting_type = new_type;
        break;
    }
    case IS: {
        MAKE_PRIMITIVE(STYPE_BOOL, false, new_type, allocator.memory_alloc, {
            RC_RELEASE(lhs_type, allocator.free_alloc);
            RC_RELEASE(rhs_type, allocator.free_alloc);
        });
        resulting_type = new_type;
        break;
    }
    case IN:
        // TODO
        RC_RELEASE(lhs_type, allocator.free_alloc);
        RC_RELEASE(rhs_type, allocator.free_alloc);
        return NOT_IMPLEMENTED;
    case DOT_DOT:
    case DOT_DOT_EQ:
        // TODO
        RC_RELEASE(lhs_type, allocator.free_alloc);
        RC_RELEASE(rhs_type, allocator.free_alloc);
        return NOT_IMPLEMENTED;
    case ORELSE:
        if (!lhs_type->nullable || rhs_type->nullable) {
            const Status error_code =
                !lhs_type->nullable ? ILLEGAL_LHS_INFIX_OPERAND : ILLEGAL_RHS_INFIX_OPERAND;
            PUT_STATUS_PROPAGATE(errors, error_code, start_token, {
                RC_RELEASE(lhs_type, allocator.free_alloc);
                RC_RELEASE(rhs_type, allocator.free_alloc);
            });
        }

        if (!type_assignable(lhs_type, rhs_type)) {
            PUT_STATUS_PROPAGATE(errors, TYPE_MISMATCH, start_token, {
                RC_RELEASE(lhs_type, allocator.free_alloc);
                RC_RELEASE(rhs_type, allocator.free_alloc);
            });
        }

        // Copy the right hand side since the result cannot be nullable
        TRY_DO(semantic_type_copy(&resulting_type, rhs_type, allocator), {
            RC_RELEASE(lhs_type, allocator.free_alloc);
            RC_RELEASE(rhs_type, allocator.free_alloc);
        });
        break;
    default:
        UNREACHABLE;
    }

    assert(resulting_type);
    parent->analyzed_type = resulting_type;

    RC_RELEASE(lhs_type, allocator.free_alloc);
    RC_RELEASE(rhs_type, allocator.free_alloc);
    return SUCCESS;
}
