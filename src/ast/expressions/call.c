#include <assert.h>

#include "ast/ast.h"
#include "ast/expressions/call.h"

void free_call_expression_list(ArrayList* arguments, free_alloc_fn free_alloc) {
    assert(arguments && arguments->data);
    assert(free_alloc);

    CallArgument argument;
    for (size_t i = 0; i < arguments->length; i++) {
        UNREACHABLE_IF_ERROR(array_list_get(arguments, i, &argument));
        NODE_VIRTUAL_FREE(argument.argument, free_alloc);
    }

    array_list_deinit(arguments);
}

TRY_STATUS call_expression_create(Token            start_token,
                                  Expression*      function,
                                  ArrayList        arguments,
                                  ArrayList        generics,
                                  CallExpression** call_expr,
                                  memory_alloc_fn  memory_alloc) {
    assert(memory_alloc);
    assert(arguments.item_size == sizeof(CallArgument));

    CallExpression* call = memory_alloc(sizeof(CallExpression));
    if (!call) {
        return ALLOCATION_FAILED;
    }

    *call = (CallExpression){
        .base      = EXPRESSION_INIT(CALL_VTABLE, start_token),
        .function  = function,
        .arguments = arguments,
        .generics  = generics,
    };

    *call_expr = call;
    return SUCCESS;
}

void call_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    ASSERT_NODE(node);
    assert(free_alloc);

    CallExpression* call = (CallExpression*)node;
    NODE_VIRTUAL_FREE(call->function, free_alloc);
    free_call_expression_list(&call->arguments, free_alloc);
    free_expression_list(&call->generics, free_alloc);

    free_alloc(call);
}

TRY_STATUS
call_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    ASSERT_NODE(node);
    if (!sb) {
        return NULL_PARAMETER;
    }

    CallExpression* call     = (CallExpression*)node;
    Node*           function = (Node*)call->function;
    PROPAGATE_IF_ERROR(function->vtable->reconstruct(function, symbol_map, sb));
    PROPAGATE_IF_ERROR(string_builder_append(sb, '('));

    CallArgument argument;
    for (size_t i = 0; i < call->arguments.length; i++) {
        UNREACHABLE_IF_ERROR(array_list_get(&call->arguments, i, &argument));
        if (argument.is_ref) {
            PROPAGATE_IF_ERROR(string_builder_append_str_z(sb, "ref "));
        }
        Node* arg_node = (Node*)argument.argument;
        PROPAGATE_IF_ERROR(arg_node->vtable->reconstruct(arg_node, symbol_map, sb));

        if (i != call->arguments.length - 1) {
            PROPAGATE_IF_ERROR(string_builder_append_str_z(sb, ", "));
        }
    }

    PROPAGATE_IF_ERROR(string_builder_append(sb, ')'));

    if (call->generics.length > 0) {
        PROPAGATE_IF_ERROR(string_builder_append_str_z(sb, " with "));
    }
    PROPAGATE_IF_ERROR(generics_reconstruct(&call->generics, symbol_map, sb));

    return SUCCESS;
}
