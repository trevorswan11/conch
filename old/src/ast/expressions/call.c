#include <assert.h>

#include "ast/ast.h"
#include "ast/expressions/call.h"

#include "semantic/context.h"

#include "util/containers/string_builder.h"

void free_call_expression_list(ArrayList* arguments, Allocator* allocator) {
    assert(arguments && arguments->data);
    ASSERT_ALLOCATOR_PTR(allocator);

    ArrayListConstIterator it = array_list_const_iterator_init(arguments);
    CallArgument           argument;
    while (array_list_const_iterator_has_next(&it, &argument)) {
        NODE_VIRTUAL_FREE(argument.argument, allocator);
    }

    array_list_deinit(arguments);
}

[[nodiscard]] Status call_expression_create(Token            start_token,
                                            Expression*      function,
                                            ArrayList        arguments,
                                            ArrayList        generics,
                                            CallExpression** call_expr,
                                            Allocator*       allocator) {
    ASSERT_ALLOCATOR_PTR(allocator);
    assert(arguments.item_size == sizeof(CallArgument));
    ASSERT_EXPRESSION(function);

    CallExpression* call = ALLOCATOR_PTR_MALLOC(allocator, sizeof(*call));
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

void call_expression_destroy(Node* node, Allocator* allocator) {
    if (!node) { return; }
    ASSERT_ALLOCATOR_PTR(allocator);

    CallExpression* call = (CallExpression*)node;
    NODE_VIRTUAL_FREE(call->function, allocator);
    free_call_expression_list(&call->arguments, allocator);
    free_expression_list(&call->generics, allocator);

    ALLOCATOR_PTR_FREE(allocator, call);
}

[[nodiscard]] Status
call_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
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

[[nodiscard]] Status call_expression_analyze(Node*                             node,
                                             [[maybe_unused]] SemanticContext* parent,
                                             [[maybe_unused]] ArrayList*       errors) {
    ASSERT_EXPRESSION(node);
    assert(parent && errors);

    [[maybe_unused]] CallExpression* call = (CallExpression*)node;
    ASSERT_EXPRESSION(call->function);

    return NOT_IMPLEMENTED;
}
