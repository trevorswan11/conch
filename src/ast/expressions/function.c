#include <assert.h>

#include "parser/expression_parsers.h"

#include "ast/ast.h"
#include "ast/expressions/function.h"
#include "ast/expressions/type.h"
#include "ast/statements/block.h"

#include "semantic/context.h"

#include "util/containers/string_builder.h"

void free_parameter_list(ArrayList* parameters, Allocator* allocator) {
    ASSERT_ALLOCATOR(parameters->allocator);

    ArrayListConstIterator it = array_list_const_iterator_init(parameters);
    Parameter              parameter;
    while (array_list_const_iterator_has_next(&it, &parameter)) {
        NODE_VIRTUAL_FREE(parameter.ident, allocator);
        NODE_VIRTUAL_FREE(parameter.type, allocator);
        NODE_VIRTUAL_FREE(parameter.default_value, allocator);
    }

    array_list_deinit(parameters);
}

[[nodiscard]] Status
reconstruct_parameter_list(ArrayList* parameters, const HashMap* symbol_map, StringBuilder* sb) {
    assert(parameters && symbol_map && sb);

    ArrayListConstIterator it = array_list_const_iterator_init(parameters);
    Parameter              parameter;
    while (array_list_const_iterator_has_next(&it, &parameter)) {
        if (parameter.is_ref) { TRY(string_builder_append_str_z(sb, "ref ")); }

        ASSERT_EXPRESSION(parameter.ident);
        TRY(NODE_VIRTUAL_RECONSTRUCT(parameter.ident, symbol_map, sb));
        TRY(string_builder_append_str_z(sb, ": "));

        ASSERT_EXPRESSION(parameter.type);
        TRY(NODE_VIRTUAL_RECONSTRUCT(parameter.type, symbol_map, sb));

        if (parameter.default_value) {
            ASSERT_EXPRESSION(parameter.default_value);
            TRY(string_builder_append_str_z(sb, " = "));
            TRY(NODE_VIRTUAL_RECONSTRUCT(parameter.default_value, symbol_map, sb));
        }

        if (!array_list_const_iterator_exhausted(&it)) {
            TRY(string_builder_append_str_z(sb, ", "));
        }
    }

    return SUCCESS;
}

[[nodiscard]] Status function_expression_create(Token                start_token,
                                                ArrayList            generics,
                                                ArrayList            parameters,
                                                TypeExpression*      return_type,
                                                BlockStatement*      body,
                                                FunctionExpression** function_expr,
                                                Allocator*           allocator) {
    ASSERT_ALLOCATOR_PTR(allocator);
    assert(generics.item_size == sizeof(Expression*));
    assert(parameters.item_size == sizeof(Parameter));
    ASSERT_EXPRESSION(return_type);
    ASSERT_STATEMENT(body);

    FunctionExpression* func = ALLOCATOR_PTR_MALLOC(allocator, sizeof(FunctionExpression));
    if (!func) { return ALLOCATION_FAILED; }

    *func = (FunctionExpression){
        .base        = EXPRESSION_INIT(FUNCTION_VTABLE, start_token),
        .generics    = generics,
        .parameters  = parameters,
        .return_type = return_type,
        .body        = body,
    };

    *function_expr = func;
    return SUCCESS;
}

void function_expression_destroy(Node* node, Allocator* allocator) {
    if (!node) { return; }
    ASSERT_ALLOCATOR_PTR(allocator);

    FunctionExpression* func = (FunctionExpression*)node;
    free_expression_list(&func->generics, allocator);
    free_parameter_list(&func->parameters, allocator);
    NODE_VIRTUAL_FREE(func->body, allocator);
    NODE_VIRTUAL_FREE(func->return_type, allocator);

    ALLOCATOR_PTR_FREE(allocator, func);
}

[[nodiscard]] Status
function_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    ASSERT_EXPRESSION(node);
    assert(sb);

    FunctionExpression* func = (FunctionExpression*)node;
    TRY(string_builder_append_str_z(sb, "fn"));
    TRY(generics_reconstruct(&func->generics, symbol_map, sb));
    TRY(string_builder_append(sb, '('));
    TRY(reconstruct_parameter_list(&func->parameters, symbol_map, sb));
    TRY(string_builder_append_str_z(sb, "): "));

    ASSERT_STATEMENT(func->return_type);
    TRY(NODE_VIRTUAL_RECONSTRUCT(func->return_type, symbol_map, sb));
    TRY(string_builder_append(sb, ' '));

    ASSERT_STATEMENT(func->body);
    TRY(NODE_VIRTUAL_RECONSTRUCT(func->body, symbol_map, sb));

    return SUCCESS;
}

[[nodiscard]] Status function_expression_analyze(Node*                             node,
                                                 [[maybe_unused]] SemanticContext* parent,
                                                 [[maybe_unused]] ArrayList*       errors) {
    ASSERT_EXPRESSION(node);
    assert(parent && errors);

    [[maybe_unused]] FunctionExpression* func = (FunctionExpression*)node;
    ASSERT_STATEMENT(func->body);

    return NOT_IMPLEMENTED;
}
