#include <assert.h>

#include "ast/ast.h"
#include "ast/expressions/infix.h"

#include "semantic/context.h"

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

    MAYBE_UNUSED(start_token);

    // Subsets of operators have different behaviors and restrictions
    switch (infix->op) {
    case PLUS:
    case MINUS:
    case STAR:
    case SLASH:
    case PERCENT:
    case STAR_STAR:
    case LT:
    case LTEQ:
    case GT:
    case GTEQ:
    case EQ:
    case NEQ:
    case BOOLEAN_AND:
    case BOOLEAN_OR:
    case AND:
    case OR:
    case XOR:
    case SHR:
    case SHL:
    case IS:
    case IN:
    case DOT_DOT:
    case DOT_DOT_EQ:
    case ORELSE:
    default:
        UNREACHABLE;
    }

    RC_RELEASE(lhs_type, allocator.free_alloc);
    RC_RELEASE(rhs_type, allocator.free_alloc);
    return NOT_IMPLEMENTED;
}
