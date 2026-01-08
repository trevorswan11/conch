#include <assert.h>

#include "ast/ast.h"
#include "ast/expressions/prefix.h"

#include "semantic/context.h"
#include "semantic/type.h"

#include "util/containers/string_builder.h"
#include "util/status.h"

[[nodiscard]] Status prefix_expression_create(Token              start_token,
                                              Expression*        rhs,
                                              PrefixExpression** prefix_expr,
                                              Allocator*         allocator) {
    ASSERT_ALLOCATOR_PTR(allocator);
    assert(start_token.slice.ptr);
    assert(start_token.type == BANG || start_token.type == NOT || start_token.type == MINUS);
    ASSERT_EXPRESSION(rhs);

    PrefixExpression* prefix = ALLOCATOR_PTR_MALLOC(allocator, sizeof(*prefix));
    if (!prefix) { return ALLOCATION_FAILED; }

    *prefix = (PrefixExpression){
        .base = EXPRESSION_INIT(PREFIX_VTABLE, start_token),
        .rhs  = rhs,
    };

    *prefix_expr = prefix;
    return SUCCESS;
}

void prefix_expression_destroy(Node* node, Allocator* allocator) {
    if (!node) { return; }
    ASSERT_ALLOCATOR_PTR(allocator);

    PrefixExpression* prefix = (PrefixExpression*)node;
    NODE_VIRTUAL_FREE(prefix->rhs, allocator);

    ALLOCATOR_PTR_FREE(allocator, prefix);
}

[[nodiscard]] Status
prefix_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    ASSERT_EXPRESSION(node);
    assert(sb);

    PrefixExpression* prefix = (PrefixExpression*)node;
    Slice             op     = poll_tt_symbol(symbol_map, node->start_token.type);

    if (group_expressions) { TRY(string_builder_append(sb, '(')); }

    TRY(string_builder_append_slice(sb, op));
    ASSERT_EXPRESSION(prefix->rhs);
    TRY(NODE_VIRTUAL_RECONSTRUCT(prefix->rhs, symbol_map, sb));

    if (group_expressions) { TRY(string_builder_append(sb, ')')); }

    return SUCCESS;
}

[[nodiscard]] Status
prefix_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors) {
    ASSERT_EXPRESSION(node);
    assert(parent && errors);

    const Token start_token = node->start_token;
    Allocator*  allocator   = semantic_context_allocator(parent);

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
        MAKE_PRIMITIVE(
            STYPE_BOOL, false, resulting_type, allocator, RC_RELEASE(operand_type, allocator));

        RC_RELEASE(operand_type, allocator);
        parent->analyzed_type = resulting_type;
        return SUCCESS;
    }

    // It doesn't make sense to operate on types or potential nil values
    if (!operand_type->valued || operand_type->nullable) {
        PUT_STATUS_PROPAGATE(
            errors, ILLEGAL_PREFIX_OPERAND, value_tok, RC_RELEASE(operand_type, allocator));
    }

    SemanticType*         resulting_type = nullptr;
    const SemanticTypeTag operand_tag    = operand_type->tag;
    switch (prefix_op) {
    case BANG:
        if (semantic_type_is_primitive(operand_type) || operand_type->nullable) {
            MAKE_PRIMITIVE(
                STYPE_BOOL, false, new_type, allocator, RC_RELEASE(operand_type, allocator));
            resulting_type = new_type;
            break;
        }

        // User defined types are not implicitly convertible to bools
        PUT_STATUS_PROPAGATE(errors, ILLEGAL_PREFIX_OPERAND, value_tok, {
            RC_RELEASE(resulting_type, allocator);
            RC_RELEASE(operand_type, allocator);
        });
    case NOT:
        if (semantic_type_is_integer(operand_type)) {
            MAKE_PRIMITIVE(
                operand_tag, false, new_type, allocator, RC_RELEASE(operand_type, allocator));
            resulting_type = new_type;
            break;
        }

        PUT_STATUS_PROPAGATE(errors, ILLEGAL_PREFIX_OPERAND, value_tok, {
            RC_RELEASE(resulting_type, allocator);
            RC_RELEASE(operand_type, allocator);
        });
    case MINUS:
        if (semantic_type_is_arithmetic(operand_type)) {
            MAKE_PRIMITIVE(
                operand_tag, false, new_type, allocator, RC_RELEASE(operand_type, allocator));
            resulting_type = new_type;
            break;
        }

        PUT_STATUS_PROPAGATE(errors, ILLEGAL_PREFIX_OPERAND, value_tok, {
            RC_RELEASE(resulting_type, allocator);
            RC_RELEASE(operand_type, allocator);
        });
    default:
        UNREACHABLE;
    }

    RC_RELEASE(operand_type, allocator);
    parent->analyzed_type = resulting_type;
    return SUCCESS;
}
