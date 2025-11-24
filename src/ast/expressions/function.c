#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include "lexer/token.h"

#include "parser/expression_parsers.h"
#include "parser/parser.h"

#include "ast/expressions/function.h"
#include "ast/statements/block.h"

#include "util/allocator.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"
#include "util/mem.h"
#include "util/status.h"

TRY_STATUS allocate_parameter_list(Parser* p, ArrayList* parameters, bool* contains_default_param) {
    if (!p || !parameters) {
        return NULL_PARAMETER;
    }
    ASSERT_ALLOCATOR(p->allocator);
    Allocator allocator = p->allocator;

    ArrayList list;
    PROPAGATE_IF_ERROR(array_list_init_allocator(&list, 2, sizeof(Parameter), allocator));

    bool some_initialized = false;
    if (parser_peek_token_is(p, RPAREN)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
    } else {
        PROPAGATE_IF_ERROR_DO(parser_next_token(p), array_list_deinit(&list));
        while (!parser_current_token_is(p, RPAREN)) {
            // Every parameter must have an identifier and a type
            IdentifierExpression* ident;
            PROPAGATE_IF_ERROR_DO(
                identifier_expression_create(
                    p->current_token, &ident, allocator.memory_alloc, allocator.free_alloc),
                free_parameter_list(&list));

            Expression* type_expr;
            bool        initalized;
            PROPAGATE_IF_ERROR_DO(type_expression_parse(p, &type_expr, &initalized), {
                identifier_expression_destroy((Node*)ident, allocator.free_alloc);
                free_parameter_list(&list);
            });

            TypeExpression* type = (TypeExpression*)type_expr;
            if (type->type.tag == IMPLICIT) {
                identifier_expression_destroy((Node*)ident, allocator.free_alloc);
                type_expression_destroy((Node*)type, allocator.free_alloc);
                free_parameter_list(&list);
                return UNEXPECTED_TOKEN;
            }

            // Parse the default value based on the types knowledge of it
            Expression* default_value = NULL;
            if (initalized) {
                some_initialized = some_initialized || initalized;

                PROPAGATE_IF_ERROR_DO(expression_parse(p, LOWEST, &default_value), {
                    identifier_expression_destroy((Node*)ident, allocator.free_alloc);
                    type_expression_destroy((Node*)type, allocator.free_alloc);
                    free_parameter_list(&list);
                });

                // Expression parsing moves up to the end of the expression, so pass it
                PROPAGATE_IF_ERROR_DO(parser_next_token(p), {
                    identifier_expression_destroy((Node*)ident, allocator.free_alloc);
                    type_expression_destroy((Node*)type, allocator.free_alloc);
                    Node* default_node = (Node*)default_value;
                    default_node->vtable->destroy(default_node, p->allocator.free_alloc);
                    free_parameter_list(&list);
                });
            }

            Parameter parameter = {
                .ident         = ident,
                .type          = type,
                .default_value = default_value,
            };

            PROPAGATE_IF_ERROR_DO(array_list_push(&list, &parameter), {
                identifier_expression_destroy((Node*)ident, allocator.free_alloc);
                type_expression_destroy((Node*)type, allocator.free_alloc);
                if (default_value) {
                    Node* default_node = (Node*)default_value;
                    default_node->vtable->destroy(default_node, allocator.free_alloc);
                }

                free_parameter_list(&list);
            });

            // Parsing a type may move up to the closing parentheses
            if (parser_current_token_is(p, RPAREN)) {
                break;
            }

            // Consume the comma and advance past it if theres not a closing delim
            PROPAGATE_IF_ERROR_DO(parser_expect_current(p, COMMA), free_parameter_list(&list););
        }
    }

    // The caller may not care about default parameter presence
    if (contains_default_param) {
        *contains_default_param = some_initialized;
    }

    *parameters = list;
    return SUCCESS;
}

void free_parameter_list(ArrayList* parameters) {
    ASSERT_ALLOCATOR(parameters->allocator);
    free_alloc_fn free_alloc = parameters->allocator.free_alloc;

    for (size_t i = 0; i < parameters->length; i++) {
        Parameter parameter;
        UNREACHABLE_IF_ERROR(array_list_get(parameters, i, &parameter));

        if (parameter.ident) {
            Node* ident = (Node*)parameter.ident;
            ident->vtable->destroy(ident, free_alloc);
            ident = NULL;
        }

        if (parameter.type) {
            Node* type = (Node*)parameter.type;
            type->vtable->destroy(type, free_alloc);
            type = NULL;
        }

        if (parameter.default_value) {
            Node* default_value = (Node*)parameter.default_value;
            default_value->vtable->destroy(default_value, free_alloc);
            default_value = NULL;
        }
    }

    array_list_deinit(parameters);
}

TRY_STATUS
reconstruct_parameter_list(ArrayList* parameters, const HashMap* symbol_map, StringBuilder* sb) {
    if (!parameters || !symbol_map || !sb) {
        return NULL_PARAMETER;
    }

    for (size_t i = 0; i < parameters->length; i++) {
        Parameter parameter;
        UNREACHABLE_IF_ERROR(array_list_get(parameters, i, &parameter));

        assert(parameter.ident);
        Node* ident = (Node*)parameter.ident;
        PROPAGATE_IF_ERROR(ident->vtable->reconstruct(ident, symbol_map, sb));

        assert(parameter.type);
        Node* type = (Node*)parameter.type;
        PROPAGATE_IF_ERROR(type->vtable->reconstruct(type, symbol_map, sb));

        if (parameter.default_value) {
            PROPAGATE_IF_ERROR(string_builder_append_many(sb, " = ", 3));
            Node* default_value = (Node*)parameter.default_value;
            PROPAGATE_IF_ERROR(default_value->vtable->reconstruct(default_value, symbol_map, sb));
        }
    }

    return SUCCESS;
}

TRY_STATUS function_expression_create(ArrayList            parameters,
                                      BlockStatement*      body,
                                      FunctionExpression** function_expr,
                                      memory_alloc_fn      memory_alloc) {
    assert(parameters.data);
    if (!body) {
        return NULL_PARAMETER;
    }

    assert(memory_alloc);
    FunctionExpression* func = memory_alloc(sizeof(FunctionExpression));
    if (!func) {
        return ALLOCATION_FAILED;
    }

    *func = (FunctionExpression){
        .base       = EXPRESSION_INIT(FUNCTION_VTABLE),
        .parameters = parameters,
        .body       = body,
    };

    *function_expr = func;
    return SUCCESS;
}

void function_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    ASSERT_NODE(node);
    assert(free_alloc);
    FunctionExpression* func = (FunctionExpression*)node;
    free_parameter_list(&func->parameters);

    if (func->body) {
        Node* body = (Node*)func->body;
        body->vtable->destroy(body, free_alloc);
        body = NULL;
    }

    free_alloc(func);
}

Slice function_expression_token_literal(Node* node) {
    MAYBE_UNUSED(node);
    return slice_from_str_z(token_type_name(FUNCTION));
}

TRY_STATUS
function_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    ASSERT_NODE(node);
    if (!sb) {
        return NULL_PARAMETER;
    }

    FunctionExpression* func = (FunctionExpression*)node;
    PROPAGATE_IF_ERROR(string_builder_append_many(sb, "fn(", 3));
    PROPAGATE_IF_ERROR(reconstruct_parameter_list(&func->parameters, symbol_map, sb));

    PROPAGATE_IF_ERROR(string_builder_append_many(sb, ") ", 2));
    Node* body = (Node*)func->body;
    PROPAGATE_IF_ERROR(body->vtable->reconstruct(body, symbol_map, sb));

    return SUCCESS;
}
