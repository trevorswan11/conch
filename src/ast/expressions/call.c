#include <assert.h>

#include "ast/ast.h"
#include "ast/expressions/call.h"

#include "semantic/context.h"

#include "util/containers/string_builder.h"

void free_call_expression_list(ArrayList* arguments, free_alloc_fn free_alloc) {
    assert(arguments && arguments->data);
    assert(free_alloc);

    ArrayListConstIterator it = array_list_const_iterator_init(arguments);
    CallArgument           argument;
    while (array_list_const_iterator_has_next(&it, &argument)) {
        NODE_VIRTUAL_FREE(argument.argument, free_alloc);
    }

    array_list_deinit(arguments);
}

[[nodiscard]] Status call_expression_create(Token            start_token,
                                        Expression*      function,
                                        ArrayList        arguments,
                                        ArrayList        generics,
                                        CallExpression** call_expr,
                                        memory_alloc_fn  memory_alloc) {
    assert(memory_alloc);
    assert(arguments.item_size == sizeof(CallArgument));
    ASSERT_EXPRESSION(function);

    CallExpression* call = memory_alloc(sizeof(CallExpression));
    if (!call) { return ALLOCATION_FAILED; }

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
    if (!node) { return; }
    assert(free_alloc);

    CallExpression* call = (CallExpression*)node;
    NODE_VIRTUAL_FREE(call->function, free_alloc);
    free_call_expression_list(&call->arguments, free_alloc);
    free_expression_list(&call->generics, free_alloc);

    free_alloc(call);
}

[[nodiscard]] Status call_expression_reconstruct(Node*          node,
                                             const HashMap* symbol_map,
                                             StringBuilder* sb) {
    ASSERT_EXPRESSION(node);
    assert(sb);

    CallExpression* call = (CallExpression*)node;
    ASSERT_EXPRESSION(call->function);
    TRY(NODE_VIRTUAL_RECONSTRUCT(call->function, symbol_map, sb));
    TRY(string_builder_append(sb, '('));

    ArrayListConstIterator it = array_list_const_iterator_init(&call->arguments);
    CallArgument           argument;
    while (array_list_const_iterator_has_next(&it, &argument)) {
        if (argument.is_ref) { TRY(string_builder_append_str_z(sb, "ref ")); }

        ASSERT_EXPRESSION(argument.argument);
        TRY(NODE_VIRTUAL_RECONSTRUCT(argument.argument, symbol_map, sb));

        if (!array_list_const_iterator_exhausted(&it)) {
            TRY(string_builder_append_str_z(sb, ", "));
        }
    }

    TRY(string_builder_append(sb, ')'));

    if (call->generics.length > 0) { TRY(string_builder_append_str_z(sb, " with ")); }
    TRY(generics_reconstruct(&call->generics, symbol_map, sb));

    return SUCCESS;
}

[[nodiscard]] Status call_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors) {
    ASSERT_EXPRESSION(node);
    assert(parent && errors);

    CallExpression* call = (CallExpression*)node;
    ASSERT_EXPRESSION(call->function);

    MAYBE_UNUSED(call);
    MAYBE_UNUSED(parent);
    MAYBE_UNUSED(errors);
    return NOT_IMPLEMENTED;
}
