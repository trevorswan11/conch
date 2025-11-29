#include <assert.h>
#include <stdio.h>

#include "ast/ast.h"
#include "ast/expressions/bool.h"
#include "ast/expressions/call.h"
#include "ast/expressions/enum.h"
#include "ast/expressions/float.h"
#include "ast/expressions/function.h"
#include "ast/expressions/identifier.h"
#include "ast/expressions/if.h"
#include "ast/expressions/infix.h"
#include "ast/expressions/integer.h"
#include "ast/expressions/prefix.h"
#include "ast/expressions/string.h"
#include "ast/expressions/type.h"
#include "ast/statements/block.h"
#include "ast/statements/declarations.h"
#include "ast/statements/jump.h"
#include "ast/statements/statement.h"

#include "parser/expression_parsers.h"
#include "parser/statement_parsers.h"

#include "util/allocator.h"
#include "util/alphanum.h"
#include "util/containers/string_builder.h"
#include "util/mem.h"

static inline TRY_STATUS record_missing_prefix(Parser* p) {
    const Token   current = p->current_token;
    StringBuilder sb;
    PROPAGATE_IF_ERROR(string_builder_init_allocator(&sb, 50, p->allocator));

    const char start[] = "No prefix parse function for ";
    PROPAGATE_IF_ERROR_DO(string_builder_append_many(&sb, start, sizeof(start) - 1),
                          string_builder_deinit(&sb));

    const char* token_literal = token_type_name(current.type);
    PROPAGATE_IF_ERROR_DO(string_builder_append_str_z(&sb, token_literal),
                          string_builder_deinit(&sb));

    const char end[] = " found";
    PROPAGATE_IF_ERROR_DO(string_builder_append_many(&sb, end, sizeof(end) - 1),
                          string_builder_deinit(&sb));

    PROPAGATE_IF_ERROR_DO(error_append_ln_col(current.line, current.column, &sb),
                          string_builder_deinit(&sb));

    MutSlice slice;
    PROPAGATE_IF_ERROR_DO(string_builder_to_string(&sb, &slice), string_builder_deinit(&sb));
    PROPAGATE_IF_ERROR_DO(array_list_push(&p->errors, &slice), string_builder_deinit(&sb));
    return SUCCESS;
}

TRY_STATUS expression_parse(Parser* p, Precedence precedence, Expression** lhs_expression) {
    assert(p);
    ASSERT_ALLOCATOR(p->allocator);

    PrefixFn prefix;
    if (!poll_prefix(p, p->current_token.type, &prefix)) {
        PROPAGATE_IF_ERROR(record_missing_prefix(p));
        return ELEMENT_MISSING;
    }
    PROPAGATE_IF_ERROR(prefix.prefix_parse(p, lhs_expression));

    while (!parser_peek_token_is(p, SEMICOLON) && precedence < parser_peek_precedence(p)) {
        InfixFn infix;
        if (!poll_infix(p, p->peek_token.type, &infix)) {
            return SUCCESS;
        }

        PROPAGATE_IF_ERROR(parser_next_token(p));
        PROPAGATE_IF_ERROR(infix.infix_parse(p, *lhs_expression, lhs_expression));
    }

    return SUCCESS;
}

TRY_STATUS identifier_expression_parse(Parser* p, Expression** expression) {
    IdentifierExpression* ident;
    PROPAGATE_IF_ERROR(identifier_expression_create(
        p->current_token, &ident, p->allocator.memory_alloc, p->allocator.free_alloc));
    *expression = (Expression*)ident;
    return SUCCESS;
}

TRY_STATUS function_definition_parse(Parser*          p,
                                     ArrayList*       parameters,
                                     TypeExpression** return_type,
                                     bool*            contains_default_param) {
    assert(parser_current_token_is(p, FUNCTION));
    PROPAGATE_IF_ERROR(parser_expect_peek(p, LPAREN));

    PROPAGATE_IF_ERROR(allocate_parameter_list(p, parameters, contains_default_param));

    PROPAGATE_IF_ERROR_DO(parser_expect_peek(p, COLON), free_parameter_list(parameters));
    const Token type_token_start = p->current_token;

    PROPAGATE_IF_ERROR_DO(explicit_type_parse(p, type_token_start, return_type), {
        IGNORE_STATUS(parser_put_status_error(
            p, MALFORMED_FUNCTION_LITERAL, type_token_start.line, type_token_start.column));
        free_parameter_list(parameters);
    });

    return SUCCESS;
}

TRY_STATUS explicit_type_parse(Parser* p, Token start_token, TypeExpression** type) {
    // Check for a question mark to allow nil
    bool is_nullable = false;
    if (parser_peek_token_is(p, WHAT)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
        is_nullable = true;
    }

    // Check for primitive before creating an identifier
    const bool is_primitive   = hash_set_contains(&p->primitives, &p->peek_token.type);
    TypeUnion  explicit_union = (TypeUnion){
         .explicit_type =
            (ExplicitType){
                 .tag = EXPLICIT_IDENT,
                 .variant =
                    (ExplicitTypeUnion){
                         .ident_type_name = NULL,
                    },
                 .nullable  = is_nullable,
                 .primitive = is_primitive,
            },
    };

    if (is_primitive || parser_peek_token_is(p, IDENT)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));

        Expression* ident_expr;
        PROPAGATE_IF_ERROR(identifier_expression_parse(p, &ident_expr));
        IdentifierExpression* ident = (IdentifierExpression*)ident_expr;

        explicit_union.explicit_type.tag     = EXPLICIT_IDENT;
        explicit_union.explicit_type.variant = (ExplicitTypeUnion){
            .ident_type_name = ident,
        };

        PROPAGATE_IF_ERROR_DO(
            type_expression_create(
                start_token, EXPLICIT, explicit_union, type, p->allocator.memory_alloc),
            identifier_expression_destroy((Node*)ident_expr, p->allocator.free_alloc));
    } else if (parser_peek_token_is(p, FUNCTION)) {
        const Token type_start = p->current_token;
        UNREACHABLE_IF_ERROR(parser_next_token(p));

        bool            contains_default_param;
        ArrayList       parameters;
        TypeExpression* return_type;
        PROPAGATE_IF_ERROR(
            function_definition_parse(p, &parameters, &return_type, &contains_default_param));

        // Default values in function types make no sense to allow, but shouldn't halt parsing
        if (contains_default_param) {
            PROPAGATE_IF_ERROR_DO(
                parser_put_status_error(
                    p, MALFORMED_FUNCTION_LITERAL, type_start.line, type_start.column),
                {
                    free_parameter_list(&parameters);
                    type_expression_destroy((Node*)return_type, p->allocator.free_alloc);
                });
        }

        explicit_union.explicit_type.tag     = EXPLICIT_FN;
        explicit_union.explicit_type.variant = (ExplicitTypeUnion){
            .function_type =
                (ExplicitFunctionType){
                    .fn_type_params = parameters,
                    .return_type    = return_type,
                },
        };

        PROPAGATE_IF_ERROR_DO(
            type_expression_create(
                start_token, EXPLICIT, explicit_union, type, p->allocator.memory_alloc),
            {
                free_parameter_list(&parameters);
                type_expression_destroy((Node*)return_type, p->allocator.free_alloc);
            });
    } else {
        return UNEXPECTED_TOKEN;
    }

    return SUCCESS;
}

TRY_STATUS type_expression_parse(Parser* p, Expression** expression, bool* initialized) {
    const Token start_token = p->current_token;
    assert(p->primitives.buffer);
    *initialized = false;

    TypeExpression* type;
    if (parser_peek_token_is(p, WALRUS)) {
        PROPAGATE_IF_ERROR(type_expression_create(
            start_token, IMPLICIT, IMPLICIT_TYPE, &type, p->allocator.memory_alloc));

        UNREACHABLE_IF_ERROR(parser_next_token(p));
        *initialized = true;
    } else if (parser_peek_token_is(p, COLON)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
        PROPAGATE_IF_ERROR(explicit_type_parse(p, start_token, &type));

        if (parser_peek_token_is(p, ASSIGN)) {
            *initialized = true;
            UNREACHABLE_IF_ERROR(parser_next_token(p));
        }
    } else {
        PROPAGATE_IF_ERROR_IS(parser_peek_error(p, COLON), REALLOCATION_FAILED);
        return UNEXPECTED_TOKEN;
    }

    // Advance again to prepare for rhs
    PROPAGATE_IF_ERROR(parser_next_token(p));

    *expression = (Expression*)type;
    return SUCCESS;
}

static inline Base integer_token_to_base(TokenType type) {
    switch (type) {
    case INT_2:
    case UINT_2:
        return BINARY;
    case INT_8:
    case UINT_8:
        return OCTAL;
    case INT_10:
    case UINT_10:
        return DECIMAL;
    case INT_16:
    case UINT_16:
        return HEXADECIMAL;
    default:
        UNREACHABLE_IF(!token_is_integer(type));
        return UNKNOWN;
    }
}

TRY_STATUS integer_literal_expression_parse(Parser* p, Expression** expression) {
    const Token start_token = p->current_token;
    assert(start_token.slice.length > 0);
    const int base = integer_token_to_base(start_token.type);

    if (token_is_signed_integer(start_token.type)) {
        int64_t      value;
        const Status parse_status =
            strntoll(start_token.slice.ptr, start_token.slice.length, base, &value);
        if (STATUS_ERR(parse_status)) {
            PROPAGATE_IF_ERROR(
                parser_put_status_error(p, parse_status, start_token.line, start_token.column));
            return parse_status;
        }

        IntegerLiteralExpression* integer;
        PROPAGATE_IF_ERROR(integer_literal_expression_create(
            start_token, value, &integer, p->allocator.memory_alloc));

        *expression = (Expression*)integer;
    } else if (token_is_unsigned_integer(start_token.type)) {
        uint64_t     value;
        const Status parse_status =
            strntoull(start_token.slice.ptr, start_token.slice.length - 1, base, &value);
        if (STATUS_ERR(parse_status)) {
            PROPAGATE_IF_ERROR(
                parser_put_status_error(p, parse_status, start_token.line, start_token.column));
            return parse_status;
        }

        UnsignedIntegerLiteralExpression* integer;
        PROPAGATE_IF_ERROR(uinteger_literal_expression_create(
            start_token, value, &integer, p->allocator.memory_alloc));

        *expression = (Expression*)integer;
    } else {
        UNREACHABLE;
        return UNEXPECTED_TOKEN;
    }

    return SUCCESS;
}

TRY_STATUS float_literal_expression_parse(Parser* p, Expression** expression) {
    const Token start_token = p->current_token;
    assert(start_token.slice.length > 0);

    double       value;
    const Status parse_status = strntod(start_token.slice.ptr,
                                        start_token.slice.length,
                                        &value,
                                        p->allocator.memory_alloc,
                                        p->allocator.free_alloc);
    if (STATUS_ERR(parse_status)) {
        PROPAGATE_IF_ERROR(
            parser_put_status_error(p, parse_status, start_token.line, start_token.column));
        return parse_status;
    }

    FloatLiteralExpression* float_expr;
    PROPAGATE_IF_ERROR(float_literal_expression_create(
        start_token, value, &float_expr, p->allocator.memory_alloc));

    *expression = (Expression*)float_expr;
    return SUCCESS;
}

TRY_STATUS prefix_expression_parse(Parser* p, Expression** expression) {
    const Token prefix_token = p->current_token;
    assert(prefix_token.slice.length > 0);
    PROPAGATE_IF_ERROR(parser_next_token(p));

    Expression* rhs;
    PROPAGATE_IF_ERROR(expression_parse(p, PREFIX, &rhs));

    PrefixExpression* prefix;
    PROPAGATE_IF_ERROR_DO(
        prefix_expression_create(prefix_token, rhs, &prefix, p->allocator.memory_alloc),
        NODE_VIRTUAL_FREE(rhs, p->allocator.free_alloc));

    *expression = (Expression*)prefix;
    return SUCCESS;
}

TRY_STATUS infix_expression_parse(Parser* p, Expression* left, Expression** expression) {
    const TokenType  op_token_type      = p->current_token.type;
    const Precedence current_precedence = parser_current_precedence(p);
    PROPAGATE_IF_ERROR(parser_next_token(p));
    ASSERT_EXPRESSION(left);

    Expression* right;
    PROPAGATE_IF_ERROR(expression_parse(p, current_precedence, &right));

    Node*            left_node = (Node*)left;
    InfixExpression* infix;
    PROPAGATE_IF_ERROR_DO(
        infix_expression_create(
            left_node->start_token, left, op_token_type, right, &infix, p->allocator.memory_alloc),
        NODE_VIRTUAL_FREE(right, p->allocator.free_alloc));

    *expression = (Expression*)infix;
    return SUCCESS;
}

TRY_STATUS bool_expression_parse(Parser* p, Expression** expression) {
    BoolLiteralExpression* boolean;
    PROPAGATE_IF_ERROR(
        bool_literal_expression_create(p->current_token, &boolean, p->allocator.memory_alloc));
    *expression = (Expression*)boolean;
    return SUCCESS;
}

TRY_STATUS string_expression_parse(Parser* p, Expression** expression) {
    StringLiteralExpression* string;
    PROPAGATE_IF_ERROR(string_literal_expression_create(p->current_token, &string, p->allocator));
    *expression = (Expression*)string;
    return SUCCESS;
}

TRY_STATUS grouped_expression_parse(Parser* p, Expression** expression) {
    UNREACHABLE_IF_ERROR(parser_next_token(p));

    Expression* inner;
    PROPAGATE_IF_ERROR(expression_parse(p, LOWEST, &inner));

    PROPAGATE_IF_ERROR_DO(parser_expect_peek(p, RPAREN),
                          NODE_VIRTUAL_FREE(inner, p->allocator.free_alloc));

    *expression = inner;
    return SUCCESS;
}

static inline TRY_STATUS _if_expression_parse_branch(Parser* p, Statement** stmt) {
    if (parser_peek_token_is(p, LBRACE)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));

        BlockStatement* alt_block;
        PROPAGATE_IF_ERROR(block_statement_parse(p, &alt_block));

        *stmt = (Statement*)alt_block;
    } else {
        PROPAGATE_IF_ERROR(parser_next_token(p));

        Statement* alt_stmt;
        PROPAGATE_IF_ERROR(parser_parse_statement(p, &alt_stmt));
        *stmt = alt_stmt;
    }

    return SUCCESS;
}

TRY_STATUS if_expression_parse(Parser* p, Expression** expression) {
    const Token start_token = p->current_token;
    PROPAGATE_IF_ERROR(parser_expect_peek(p, LPAREN));
    PROPAGATE_IF_ERROR(parser_next_token(p));

    Expression* condition;
    PROPAGATE_IF_ERROR(expression_parse(p, LOWEST, &condition));

    PROPAGATE_IF_ERROR_DO(parser_expect_peek(p, RPAREN),
                          NODE_VIRTUAL_FREE(condition, p->allocator.free_alloc));

    Statement* consequence;
    PROPAGATE_IF_ERROR_DO(_if_expression_parse_branch(p, &consequence),
                          NODE_VIRTUAL_FREE(condition, p->allocator.free_alloc));

    Statement* alternate = NULL;
    if (parser_peek_token_is(p, ELSE)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
        PROPAGATE_IF_ERROR_DO(_if_expression_parse_branch(p, &alternate), {
            NODE_VIRTUAL_FREE(condition, p->allocator.free_alloc);
            NODE_VIRTUAL_FREE(consequence, p->allocator.free_alloc);
        });
    }

    IfExpression* if_expr;
    PROPAGATE_IF_ERROR_DO(
        if_expression_create(
            start_token, condition, consequence, alternate, &if_expr, p->allocator.memory_alloc),
        {
            NODE_VIRTUAL_FREE(condition, p->allocator.free_alloc);
            NODE_VIRTUAL_FREE(consequence, p->allocator.free_alloc);
            NODE_VIRTUAL_FREE(alternate, p->allocator.free_alloc);
        });

    *expression = (Expression*)if_expr;
    return SUCCESS;
}

TRY_STATUS function_expression_parse(Parser* p, Expression** expression) {
    const Token     start_token = p->current_token;
    ArrayList       parameters;
    TypeExpression* return_type;

    PROPAGATE_IF_ERROR(function_definition_parse(p, &parameters, &return_type, NULL));
    PROPAGATE_IF_ERROR_DO(parser_expect_peek(p, LBRACE), {
        free_parameter_list(&parameters);
        type_expression_destroy((Node*)return_type, p->allocator.free_alloc);
    });

    BlockStatement* body;
    PROPAGATE_IF_ERROR_DO(block_statement_parse(p, &body), {
        free_parameter_list(&parameters);
        type_expression_destroy((Node*)return_type, p->allocator.free_alloc);
    });

    FunctionExpression* function;
    PROPAGATE_IF_ERROR_DO(
        function_expression_create(
            start_token, parameters, return_type, body, &function, p->allocator.memory_alloc),
        {
            type_expression_destroy((Node*)return_type, p->allocator.free_alloc);
            free_parameter_list(&parameters);
            block_statement_destroy((Node*)body, p->allocator.free_alloc);
        });

    *expression = (Expression*)function;
    return SUCCESS;
}

TRY_STATUS call_expression_parse(Parser* p, Expression* function, Expression** expression) {
    const Token start_token = p->current_token;

    ArrayList arguments;
    PROPAGATE_IF_ERROR(array_list_init_allocator(&arguments, 2, sizeof(Expression*), p->allocator));

    if (parser_peek_token_is(p, RPAREN)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
    } else {
        PROPAGATE_IF_ERROR_DO(parser_next_token(p), array_list_deinit(&arguments));

        // Append the first argument to prepare for comma peeking
        Expression* argument;
        PROPAGATE_IF_ERROR_DO(expression_parse(p, LOWEST, &argument),
                              array_list_deinit(&arguments));

        PROPAGATE_IF_ERROR_DO(array_list_push(&arguments, &argument), {
            NODE_VIRTUAL_FREE(argument, p->allocator.free_alloc);
            array_list_deinit(&arguments);
        });

        // Add the rest of the comma separated argument list
        while (parser_peek_token_is(p, COMMA)) {
            UNREACHABLE_IF_ERROR(parser_next_token(p));
            PROPAGATE_IF_ERROR_DO(parser_next_token(p), {
                clear_expression_list(&arguments, p->allocator.free_alloc);
                array_list_deinit(&arguments);
            });

            PROPAGATE_IF_ERROR_DO(expression_parse(p, LOWEST, &argument), {
                clear_expression_list(&arguments, p->allocator.free_alloc);
                array_list_deinit(&arguments);
            });

            PROPAGATE_IF_ERROR_DO(array_list_push(&arguments, &argument), {
                clear_expression_list(&arguments, p->allocator.free_alloc);
                array_list_deinit(&arguments);
            });
        }

        PROPAGATE_IF_ERROR_DO(parser_expect_peek(p, RPAREN), {
            clear_expression_list(&arguments, p->allocator.free_alloc);
            array_list_deinit(&arguments);
        });
    }

    CallExpression* call;
    PROPAGATE_IF_ERROR_DO(
        call_expression_create(start_token, function, arguments, &call, p->allocator.memory_alloc),
        {
            clear_expression_list(&arguments, p->allocator.free_alloc);
            array_list_deinit(&arguments);
        });

    *expression = (Expression*)call;
    return SUCCESS;
}

TRY_STATUS struct_expression_parse(Parser* p, Expression** expression) {
    MAYBE_UNUSED(p);
    MAYBE_UNUSED(expression);
    return NOT_IMPLEMENTED;
}

TRY_STATUS enum_expression_parse(Parser* p, Expression** expression) {
    const Token start_token = p->current_token;
    PROPAGATE_IF_ERROR(parser_expect_peek(p, LBRACE));

    ArrayList variants;
    PROPAGATE_IF_ERROR(array_list_init_allocator(&variants, 4, sizeof(EnumVariant), p->allocator));
    while (true) {
        PROPAGATE_IF_ERROR_DO(parser_expect_peek(p, IDENT),
                              free_enum_variant_list(&variants, p->allocator.free_alloc));

        IdentifierExpression* ident;
        PROPAGATE_IF_ERROR_DO(
            identifier_expression_create(
                p->current_token, &ident, p->allocator.memory_alloc, p->allocator.free_alloc),
            free_enum_variant_list(&variants, p->allocator.free_alloc));

        Expression* value = NULL;
        if (parser_peek_token_is(p, ASSIGN)) {
            UNREACHABLE_IF_ERROR(parser_next_token(p));
            PROPAGATE_IF_ERROR_DO(parser_next_token(p), {
                free_enum_variant_list(&variants, p->allocator.free_alloc);
                identifier_expression_destroy((Node*)ident, p->allocator.free_alloc);
            });

            PROPAGATE_IF_ERROR_DO(expression_parse(p, LOWEST, &value), {
                free_enum_variant_list(&variants, p->allocator.free_alloc);
                identifier_expression_destroy((Node*)ident, p->allocator.free_alloc);
            });
        }

        // All variants require a trailing comma!
        PROPAGATE_IF_ERROR_DO(parser_expect_peek(p, COMMA), {
            free_enum_variant_list(&variants, p->allocator.free_alloc);
            identifier_expression_destroy((Node*)ident, p->allocator.free_alloc);
            NODE_VIRTUAL_FREE(value, p->allocator.free_alloc);
        });

        EnumVariant variant = (EnumVariant){.name = ident, .value = value};
        PROPAGATE_IF_ERROR_DO(array_list_push(&variants, &variant), {
            free_enum_variant_list(&variants, p->allocator.free_alloc);
            identifier_expression_destroy((Node*)ident, p->allocator.free_alloc);
            NODE_VIRTUAL_FREE(value, p->allocator.free_alloc);
        });

        // A terminal delimiter ends the enum, and can be followed by a semicolon
        if (parser_peek_token_is(p, RBRACE)) {
            UNREACHABLE_IF_ERROR(parser_next_token(p));
            if (parser_peek_token_is(p, SEMICOLON)) {
                UNREACHABLE_IF_ERROR(parser_next_token(p));
            }

            break;
        }
    }

    EnumExpression* enum_expr;
    PROPAGATE_IF_ERROR_DO(
        enum_expression_create(start_token, variants, &enum_expr, p->allocator.memory_alloc),
        free_enum_variant_list(&variants, p->allocator.free_alloc));

    *expression = (Expression*)enum_expr;
    return SUCCESS;
}
