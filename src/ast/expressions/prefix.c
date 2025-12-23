#include <assert.h>

#include "ast/ast.h"
#include "ast/expressions/prefix.h"

#include "semantic/context.h"
#include "semantic/type.h"

#include "util/containers/string_builder.h"
#include "util/status.h"

NODISCARD Status prefix_expression_create(Token              start_token,
                                          Expression*        rhs,
                                          PrefixExpression** prefix_expr,
                                          memory_alloc_fn    memory_alloc) {
    assert(memory_alloc);
    assert(start_token.slice.ptr);
    assert(start_token.type == BANG || start_token.type == NOT || start_token.type == MINUS);
    ASSERT_EXPRESSION(rhs);

    PrefixExpression* prefix = memory_alloc(sizeof(PrefixExpression));
    if (!prefix) {
        return ALLOCATION_FAILED;
    }

    *prefix = (PrefixExpression){
        .base = EXPRESSION_INIT(PREFIX_VTABLE, start_token),
        .rhs  = rhs,
    };

    *prefix_expr = prefix;
    return SUCCESS;
}

void prefix_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    if (!node) {
        return;
    }
    assert(free_alloc);

    PrefixExpression* prefix = (PrefixExpression*)node;
    NODE_VIRTUAL_FREE(prefix->rhs, free_alloc);

    free_alloc(prefix);
}

NODISCARD Status prefix_expression_reconstruct(Node*          node,
                                               const HashMap* symbol_map,
                                               StringBuilder* sb) {
    ASSERT_EXPRESSION(node);
    assert(sb);

    PrefixExpression* prefix = (PrefixExpression*)node;
    Slice             op     = poll_tt_symbol(symbol_map, node->start_token.type);

    if (group_expressions) {
        TRY(string_builder_append(sb, '('));
    }

    TRY(string_builder_append_slice(sb, op));
    ASSERT_EXPRESSION(prefix->rhs);
    TRY(NODE_VIRTUAL_RECONSTRUCT(prefix->rhs, symbol_map, sb));

    if (group_expressions) {
        TRY(string_builder_append(sb, ')'));
    }

    return SUCCESS;
}

NODISCARD Status prefix_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors) {
    ASSERT_EXPRESSION(node);
    assert(parent && errors);

    const Token     start_token = node->start_token;
    const Allocator allocator   = semantic_context_allocator(parent);

    const TokenType prefix_op = start_token.type;
    assert(prefix_op == BANG || prefix_op == NOT || prefix_op == MINUS);

    PrefixExpression* prefix  = (PrefixExpression*)node;
    Expression*       operand = prefix->rhs;
    ASSERT_EXPRESSION(operand);

    // The resulting type is not always the same as the operand
    TRY(NODE_VIRTUAL_ANALYZE(operand, parent, errors));
    SemanticType* operand_type = semantic_context_move_analyzed(parent);

    // We can save an allocation and error check if we handle special cases now
    const Token value_tok = NODE_TOKEN(operand);
    if (prefix_op == BANG && operand_type->nullable) {
        MAKE_PRIMITIVE(STYPE_BOOL,
                       false,
                       resulting_type,
                       allocator.memory_alloc,
                       RC_RELEASE(operand_type, allocator.free_alloc));

        RC_RELEASE(operand_type, allocator.free_alloc);
        parent->analyzed_type = resulting_type;
        return SUCCESS;
    } else if (!operand_type->valued || operand_type->nullable) {
        PUT_STATUS_PROPAGATE(errors,
                             ILLEGAL_PREFIX_OPERAND,
                             value_tok,
                             RC_RELEASE(operand_type, allocator.free_alloc));
    }

    SemanticType* resulting_type;
    TRY_DO(semantic_type_create(&resulting_type, allocator.memory_alloc),
           RC_RELEASE(operand_type, allocator.free_alloc));

    const SemanticTypeTag operand_tag = operand_type->tag;
    switch (prefix_op) {
    case BANG:
        if (semantic_type_is_primitive(operand_type)) {
            resulting_type->tag      = STYPE_BOOL;
            resulting_type->variant  = SEMANTIC_DATALESS_TYPE;
            resulting_type->is_const = true;
            resulting_type->valued   = true;
            resulting_type->nullable = false;
        } else {
            // User defined types are not implicitly convertible to bools
            PUT_STATUS_PROPAGATE(errors, ILLEGAL_PREFIX_OPERAND, value_tok, {
                RC_RELEASE(resulting_type, allocator.free_alloc);
                RC_RELEASE(operand_type, allocator.free_alloc);
            });
        }

        break;
    case NOT:
        if (operand_tag == STYPE_SIGNED_INTEGER || operand_tag == STYPE_UNSIGNED_INTEGER) {
            resulting_type->tag      = operand_tag;
            resulting_type->variant  = SEMANTIC_DATALESS_TYPE;
            resulting_type->is_const = true;
            resulting_type->valued   = true;
            resulting_type->nullable = false;
        } else {
            PUT_STATUS_PROPAGATE(errors, ILLEGAL_PREFIX_OPERAND, value_tok, {
                RC_RELEASE(resulting_type, allocator.free_alloc);
                RC_RELEASE(operand_type, allocator.free_alloc);
            });
        }

        break;
    case MINUS:
        if (semantic_type_is_arithmetic(operand_type)) {
            resulting_type->tag      = operand_tag;
            resulting_type->variant  = SEMANTIC_DATALESS_TYPE;
            resulting_type->is_const = true;
            resulting_type->valued   = true;
            resulting_type->nullable = false;
        } else {
            PUT_STATUS_PROPAGATE(errors, ILLEGAL_PREFIX_OPERAND, value_tok, {
                RC_RELEASE(resulting_type, allocator.free_alloc);
                RC_RELEASE(operand_type, allocator.free_alloc);
            });
        }

        break;
    default:
        UNREACHABLE;
    }

    RC_RELEASE(operand_type, allocator.free_alloc);
    parent->analyzed_type = resulting_type;
    return SUCCESS;
}
