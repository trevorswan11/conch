#ifndef INFIX_EXPR_H
#define INFIX_EXPR_H

#include "ast/expressions/expression.h"

typedef struct InfixExpression {
    Expression  base;
    Expression* lhs;
    TokenType   op;
    Expression* rhs;
} InfixExpression;

[[nodiscard]] Status infix_expression_create(Token             start_token,
                                         Expression*       lhs,
                                         TokenType         op,
                                         Expression*       rhs,
                                         InfixExpression** infix_expr,
                                         memory_alloc_fn   memory_alloc);

void             infix_expression_destroy(Node* node, free_alloc_fn free_alloc);
[[nodiscard]] Status infix_expression_reconstruct(Node*          node,
                                              const HashMap* symbol_map,
                                              StringBuilder* sb);
[[nodiscard]] Status infix_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors);

static const ExpressionVTable INFIX_VTABLE = {
    .base =
        {
            .destroy     = infix_expression_destroy,
            .reconstruct = infix_expression_reconstruct,
            .analyze     = infix_expression_analyze,
        },
};

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
    (result) = new_type;

#define PLUS_STAR_INFIX_CASE                                                                \
    if (lhs_type->tag == STYPE_STR && semantic_type_is_primitive(rhs_type)) {               \
        if (lhs_type->nullable || rhs_type->nullable) {                                     \
            const Status error_code =                                                       \
                lhs_type->nullable ? ILLEGAL_LHS_INFIX_OPERAND : ILLEGAL_RHS_INFIX_OPERAND; \
            PUT_STATUS_PROPAGATE(errors, error_code, start_token, {                         \
                RC_RELEASE(lhs_type, allocator.free_alloc);                                 \
                RC_RELEASE(rhs_type, allocator.free_alloc);                                 \
            });                                                                             \
        }                                                                                   \
                                                                                            \
        MAKE_PRIMITIVE(STYPE_STR, false, new_type, allocator.memory_alloc, {                \
            RC_RELEASE(lhs_type, allocator.free_alloc);                                     \
            RC_RELEASE(rhs_type, allocator.free_alloc);                                     \
        });                                                                                 \
        resulting_type = new_type;                                                          \
        break;                                                                              \
    }                                                                                       \
                                                                                            \
    if (semantic_type_is_primitive(lhs_type) && rhs_type->tag == STYPE_STR) {               \
        PUT_STATUS_PROPAGATE(errors, ILLEGAL_LHS_INFIX_OPERAND, start_token, {              \
            RC_RELEASE(lhs_type, allocator.free_alloc);                                     \
            RC_RELEASE(rhs_type, allocator.free_alloc);                                     \
        });                                                                                 \
    }                                                                                       \
                                                                                            \
    FALLBACK_ARITHMETIC(resulting_type, {                                                   \
        RC_RELEASE(lhs_type, allocator.free_alloc);                                         \
        RC_RELEASE(rhs_type, allocator.free_alloc);                                         \
    });                                                                                     \
    break;

#define MODULE_BITWISE_INFIX_CASE                                                         \
    if (lhs_type->tag == STYPE_FLOATING_POINT || rhs_type->tag == STYPE_FLOATING_POINT) { \
        const Status error_code = lhs_type->tag == STYPE_FLOATING_POINT                   \
                                      ? ILLEGAL_LHS_INFIX_OPERAND                         \
                                      : ILLEGAL_RHS_INFIX_OPERAND;                        \
        PUT_STATUS_PROPAGATE(errors, error_code, start_token, {                           \
            RC_RELEASE(lhs_type, allocator.free_alloc);                                   \
            RC_RELEASE(rhs_type, allocator.free_alloc);                                   \
        });                                                                               \
    }                                                                                     \
                                                                                          \
    FALLBACK_ARITHMETIC(resulting_type, {                                                 \
        RC_RELEASE(lhs_type, allocator.free_alloc);                                       \
        RC_RELEASE(rhs_type, allocator.free_alloc);                                       \
    });                                                                                   \
    break;

#endif
