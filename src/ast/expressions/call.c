#include <assert.h>

#include "ast/ast.h"
#include "ast/expressions/call.h"

#include "semantic/context.h"

#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"

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

NODISCARD Status call_expression_create(Token            start_token,
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

NODISCARD Status call_expression_reconstruct(Node*          node,
                                             const HashMap* symbol_map,
                                             StringBuilder* sb) {
    ASSERT_NODE(node);
    assert(sb);

    CallExpression* call     = (CallExpression*)node;
    Node*           function = (Node*)call->function;
    TRY(function->vtable->reconstruct(function, symbol_map, sb));
    TRY(string_builder_append(sb, '('));

    CallArgument argument;
    for (size_t i = 0; i < call->arguments.length; i++) {
        UNREACHABLE_IF_ERROR(array_list_get(&call->arguments, i, &argument));
        if (argument.is_ref) {
            TRY(string_builder_append_str_z(sb, "ref "));
        }
        Node* arg_node = (Node*)argument.argument;
        TRY(arg_node->vtable->reconstruct(arg_node, symbol_map, sb));

        if (i != call->arguments.length - 1) {
            TRY(string_builder_append_str_z(sb, ", "));
        }
    }

    TRY(string_builder_append(sb, ')'));

    if (call->generics.length > 0) {
        TRY(string_builder_append_str_z(sb, " with "));
    }
    TRY(generics_reconstruct(&call->generics, symbol_map, sb));

    return SUCCESS;
}

NODISCARD Status call_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors) {
    assert(node && parent && errors);
    MAYBE_UNUSED(node);
    MAYBE_UNUSED(parent);
    MAYBE_UNUSED(errors);
    return NOT_IMPLEMENTED;
}
