#include <assert.h>

#include "parser/expression_parsers.h"

#include "ast/ast.h"
#include "ast/expressions/function.h"
#include "ast/expressions/identifier.h"
#include "ast/expressions/type.h"
#include "ast/statements/block.h"

NODISCARD Status allocate_parameter_list(Parser*    p,
                                         ArrayList* parameters,
                                         bool*      contains_default_param) {
    assert(p && parameters);
    ASSERT_ALLOCATOR(p->allocator);
    Allocator allocator = p->allocator;

    ArrayList list;
    TRY(array_list_init_allocator(&list, 2, sizeof(Parameter), allocator));

    bool some_initialized = false;
    if (parser_peek_token_is(p, RPAREN)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
    } else {
        TRY_DO(parser_next_token(p), array_list_deinit(&list));
        while (!parser_current_token_is(p, RPAREN)) {
            bool is_ref_argument = false;
            if (parser_current_token_is(p, REF)) {
                UNREACHABLE_IF_ERROR(parser_next_token(p));
                is_ref_argument = true;
            }

            // Every parameter must have an identifier and a type
            IdentifierExpression* ident;
            TRY_DO(identifier_expression_create(
                       p->current_token, &ident, allocator.memory_alloc, allocator.free_alloc),
                   free_parameter_list(&list, p->allocator.free_alloc));

            Expression* type_expr;
            bool        initalized;
            TRY_DO(type_expression_parse(p, &type_expr, &initalized), {
                identifier_expression_destroy((Node*)ident, allocator.free_alloc);
                free_parameter_list(&list, p->allocator.free_alloc);
            });

            TypeExpression* type = (TypeExpression*)type_expr;
            if (type->type.tag == IMPLICIT) {
                identifier_expression_destroy((Node*)ident, allocator.free_alloc);
                type_expression_destroy((Node*)type, allocator.free_alloc);
                free_parameter_list(&list, p->allocator.free_alloc);
                return UNEXPECTED_TOKEN;
            }

            // Parse the default value based on the types knowledge of it
            Expression* default_value = NULL;
            if (initalized) {
                some_initialized = some_initialized || initalized;

                TRY_DO(expression_parse(p, LOWEST, &default_value), {
                    identifier_expression_destroy((Node*)ident, allocator.free_alloc);
                    type_expression_destroy((Node*)type, allocator.free_alloc);
                    free_parameter_list(&list, p->allocator.free_alloc);
                });

                // Expression parsing moves up to the end of the expression, so pass it
                TRY_DO(parser_next_token(p), {
                    identifier_expression_destroy((Node*)ident, allocator.free_alloc);
                    type_expression_destroy((Node*)type, allocator.free_alloc);
                    NODE_VIRTUAL_FREE(default_value, p->allocator.free_alloc);
                    free_parameter_list(&list, p->allocator.free_alloc);
                });
            }

            Parameter parameter = {
                .is_ref        = is_ref_argument,
                .ident         = ident,
                .type          = type,
                .default_value = default_value,
            };

            TRY_DO(array_list_push(&list, &parameter), {
                identifier_expression_destroy((Node*)ident, allocator.free_alloc);
                type_expression_destroy((Node*)type, allocator.free_alloc);
                NODE_VIRTUAL_FREE(default_value, p->allocator.free_alloc);
                free_parameter_list(&list, p->allocator.free_alloc);
            });

            // Parsing a type may move up to the closing parentheses
            if (parser_current_token_is(p, RPAREN)) {
                break;
            }

            // Consume the comma and advance past it if theres not a closing delim
            TRY_DO(parser_expect_current(p, COMMA),
                   free_parameter_list(&list, p->allocator.free_alloc));
        }
    }

    // The caller may not care about default parameter presence
    if (contains_default_param) {
        *contains_default_param = some_initialized;
    }

    *parameters = list;
    return SUCCESS;
}

void free_parameter_list(ArrayList* parameters, free_alloc_fn free_alloc) {
    ASSERT_ALLOCATOR(parameters->allocator);

    for (size_t i = 0; i < parameters->length; i++) {
        Parameter parameter;
        UNREACHABLE_IF_ERROR(array_list_get(parameters, i, &parameter));

        if (parameter.ident) {
            identifier_expression_destroy((Node*)parameter.ident, free_alloc);
            parameter.ident = NULL;
        }

        if (parameter.type) {
            type_expression_destroy((Node*)parameter.type, free_alloc);
            parameter.type = NULL;
        }

        NODE_VIRTUAL_FREE(parameter.default_value, free_alloc);
        parameter.default_value = NULL;
    }

    array_list_deinit(parameters);
}

NODISCARD Status reconstruct_parameter_list(ArrayList*     parameters,
                                            const HashMap* symbol_map,
                                            StringBuilder* sb) {
    assert(parameters && symbol_map && sb);

    for (size_t i = 0; i < parameters->length; i++) {
        Parameter parameter;
        UNREACHABLE_IF_ERROR(array_list_get(parameters, i, &parameter));

        if (parameter.is_ref) {
            TRY(string_builder_append_str_z(sb, "ref "));
        }

        assert(parameter.ident);
        TRY(identifier_expression_reconstruct((Node*)parameter.ident, symbol_map, sb));
        TRY(string_builder_append_str_z(sb, ": "));

        assert(parameter.type);
        TRY(type_expression_reconstruct((Node*)parameter.type, symbol_map, sb));

        if (parameter.default_value) {
            TRY(string_builder_append_str_z(sb, " = "));
            Node* default_value = (Node*)parameter.default_value;
            TRY(default_value->vtable->reconstruct(default_value, symbol_map, sb));
        }
    }

    return SUCCESS;
}

NODISCARD Status function_expression_create(Token                start_token,
                                            ArrayList            generics,
                                            ArrayList            parameters,
                                            TypeExpression*      return_type,
                                            BlockStatement*      body,
                                            FunctionExpression** function_expr,
                                            memory_alloc_fn      memory_alloc) {
    assert(return_type && body);
    assert(parameters.item_size == sizeof(Parameter));

    assert(memory_alloc);
    FunctionExpression* func = memory_alloc(sizeof(FunctionExpression));
    if (!func) {
        return ALLOCATION_FAILED;
    }

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

void function_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    ASSERT_NODE(node);
    assert(free_alloc);

    FunctionExpression* func = (FunctionExpression*)node;
    free_expression_list(&func->generics, free_alloc);
    free_parameter_list(&func->parameters, free_alloc);
    NODE_VIRTUAL_FREE(func->body, free_alloc);
    NODE_VIRTUAL_FREE(func->return_type, free_alloc);

    free_alloc(func);
}

NODISCARD Status function_expression_reconstruct(Node*          node,
                                                 const HashMap* symbol_map,
                                                 StringBuilder* sb) {
    ASSERT_NODE(node);
    assert(sb);

    FunctionExpression* func = (FunctionExpression*)node;
    TRY(string_builder_append_str_z(sb, "fn"));
    TRY(generics_reconstruct(&func->generics, symbol_map, sb));
    TRY(string_builder_append(sb, '('));
    TRY(reconstruct_parameter_list(&func->parameters, symbol_map, sb));

    TRY(string_builder_append_str_z(sb, ") "));
    TRY(block_statement_reconstruct((Node*)func->body, symbol_map, sb));

    return SUCCESS;
}
