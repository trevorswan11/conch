#include <assert.h>

#include "ast/ast.h"
#include "ast/expressions/infix.h"

#include "semantic/context.h"
#include "semantic/type.h"

#include "util/containers/string_builder.h"

[[nodiscard]] Status infix_expression_create(Token             start_token,
                                         Expression*       lhs,
                                         TokenType         op,
                                         Expression*       rhs,
                                         InfixExpression** infix_expr,
                                         memory_alloc_fn   memory_alloc) {
    assert(memory_alloc);
    assert(lhs && rhs);

    InfixExpression* infix = memory_alloc(sizeof(InfixExpression));
    if (!infix) { return ALLOCATION_FAILED; }

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
    if (!node) { return; }
    assert(free_alloc);

    InfixExpression* infix = (InfixExpression*)node;
    NODE_VIRTUAL_FREE(infix->lhs, free_alloc);
    NODE_VIRTUAL_FREE(infix->rhs, free_alloc);

    free_alloc(infix);
}

[[nodiscard]] Status infix_expression_reconstruct(Node*          node,
                                              const HashMap* symbol_map,
                                              StringBuilder* sb) {
    ASSERT_EXPRESSION(node);
    assert(sb);

    InfixExpression* infix = (InfixExpression*)node;
    Slice            op    = poll_tt_symbol(symbol_map, infix->op);

    if (group_expressions) { TRY(string_builder_append(sb, '(')); }
    ASSERT_EXPRESSION(infix->lhs);
    TRY(NODE_VIRTUAL_RECONSTRUCT(infix->lhs, symbol_map, sb));

    if (infix->op == DOT_DOT || infix->op == DOT_DOT_EQ) {
        TRY(string_builder_append_slice(sb, op));
    } else {
        TRY(string_builder_append(sb, ' '));
        TRY(string_builder_append_slice(sb, op));
        TRY(string_builder_append(sb, ' '));
    }

    ASSERT_EXPRESSION(infix->rhs);
    TRY(NODE_VIRTUAL_RECONSTRUCT(infix->rhs, symbol_map, sb));
    if (group_expressions) { TRY(string_builder_append(sb, ')')); }

    return SUCCESS;
}

[[nodiscard]] Status infix_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors) {
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
    SemanticType* resulting_type = nullptr;
    switch (infix->op) {
    case PLUS:
    case STAR: {
        PLUS_STAR_INFIX_CASE
    }
    case AND:
    case OR:
    case XOR:
    case SHR:
    case SHL:
    case PERCENT: {
        MODULE_BITWISE_INFIX_CASE
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
    case IN: {
        // In is just a linear equality check so this aligns with EQ and NEQ
        const bool lhs_ok = lhs_type->tag == STYPE_NIL || semantic_type_is_primitive(lhs_type);
        const bool rhs_ok = rhs_type->tag == STYPE_ARRAY && !rhs_type->nullable;
        if (!lhs_ok || !rhs_ok) {
            PUT_STATUS_PROPAGATE(errors,
                                 !lhs_ok ? ILLEGAL_LHS_INFIX_OPERAND : ILLEGAL_RHS_INFIX_OPERAND,
                                 start_token,
                                 {
                                     RC_RELEASE(lhs_type, allocator.free_alloc);
                                     RC_RELEASE(rhs_type, allocator.free_alloc);
                                 });
        }

        // The search type must be compatible with the inner type
        SemanticType* inner_array_type = rhs_type->variant.array_type->inner_type;
        if (!type_assignable(lhs_type, inner_array_type)) {
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
    case DOT_DOT:
    case DOT_DOT_EQ: {
        // LHS & RHS must be integers or bytes and non-nullable
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

        TRY_DO(semantic_type_create(&resulting_type, allocator.memory_alloc), {
            RC_RELEASE(lhs_type, allocator.free_alloc);
            RC_RELEASE(rhs_type, allocator.free_alloc);
        });

        SemanticType*      inner_type = rc_retain(lhs_type);
        SemanticArrayType* array_type;
        TRY_DO(semantic_array_create(STYPE_ARRAY_RANGE,
                                     (SemanticArrayUnion){.inclusive = infix->op == DOT_DOT_EQ},
                                     inner_type,
                                     &array_type,
                                     allocator.memory_alloc),
               {
                   RC_RELEASE(inner_type, allocator.free_alloc);
                   RC_RELEASE(resulting_type, allocator.free_alloc);
                   RC_RELEASE(lhs_type, allocator.free_alloc);
                   RC_RELEASE(rhs_type, allocator.free_alloc);
               });

        resulting_type->tag      = STYPE_ARRAY;
        resulting_type->variant  = (SemanticTypeUnion){.array_type = array_type};
        resulting_type->is_const = true;
        resulting_type->valued   = true;
        resulting_type->nullable = false;
        break;
    }
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
