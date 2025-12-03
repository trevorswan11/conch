#include <assert.h>

#include "lexer/token.h"

#include "parser/expression_parsers.h"
#include "parser/parser.h"
#include "parser/precedence.h"
#include "parser/statement_parsers.h"

#include "ast/ast.h"
#include "ast/expressions/bool.h"
#include "ast/expressions/call.h"
#include "ast/expressions/enum.h"
#include "ast/expressions/expression.h"
#include "ast/expressions/float.h"
#include "ast/expressions/function.h"
#include "ast/expressions/identifier.h"
#include "ast/expressions/if.h"
#include "ast/expressions/infix.h"
#include "ast/expressions/integer.h"
#include "ast/expressions/match.h"
#include "ast/expressions/prefix.h"
#include "ast/expressions/single.h"
#include "ast/expressions/string.h"
#include "ast/expressions/struct.h"
#include "ast/expressions/type.h"
#include "ast/node.h"
#include "ast/statements/block.h"
#include "ast/statements/statement.h"

#include "util/allocator.h"
#include "util/alphanum.h"
#include "util/containers/array_list.h"
#include "util/containers/string_builder.h"
#include "util/mem.h"
#include "util/status.h"

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
    } else {
        const Token type_start = p->current_token;
        UNREACHABLE_IF_ERROR(parser_next_token(p));

        switch (p->current_token.type) {
        case FUNCTION: {
            bool            contains_default_param;
            ArrayList       parameters;
            TypeExpression* return_type;
            PROPAGATE_IF_ERROR(
                function_definition_parse(p, &parameters, &return_type, &contains_default_param));

            // Default values in function types make no sense to allow
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
            break;
        }
        case STRUCT: {
            Expression* struct_expr;
            PROPAGATE_IF_ERROR(struct_expression_parse(p, &struct_expr));
            StructExpression* struct_type = (StructExpression*)struct_expr;

            explicit_union.explicit_type.tag     = EXPLICIT_STRUCT;
            explicit_union.explicit_type.variant = (ExplicitTypeUnion){
                .struct_type = struct_type,
            };

            PROPAGATE_IF_ERROR_DO(
                type_expression_create(
                    start_token, EXPLICIT, explicit_union, type, p->allocator.memory_alloc),
                struct_expression_destroy((Node*)struct_type, p->allocator.free_alloc));
            break;
        }
        case ENUM: {
            Expression* enum_expr;
            PROPAGATE_IF_ERROR(enum_expression_parse(p, &enum_expr));
            EnumExpression* enum_type = (EnumExpression*)enum_expr;

            explicit_union.explicit_type.tag     = EXPLICIT_ENUM;
            explicit_union.explicit_type.variant = (ExplicitTypeUnion){
                .enum_type = enum_type,
            };

            PROPAGATE_IF_ERROR_DO(
                type_expression_create(
                    start_token, EXPLICIT, explicit_union, type, p->allocator.memory_alloc),
                enum_expression_destroy((Node*)enum_type, p->allocator.free_alloc));
            break;
        }
        default:
            return UNEXPECTED_TOKEN;
        }
    }

    return SUCCESS;
}

TRY_STATUS type_expression_parse(Parser* p, Expression** expression, bool* initialized) {
    const Token start_token = p->current_token;
    assert(p->primitives.buffer);
    bool is_default_initialized = false;

    TypeExpression* type;
    if (parser_peek_token_is(p, WALRUS)) {
        PROPAGATE_IF_ERROR(type_expression_create(
            start_token, IMPLICIT, IMPLICIT_TYPE, &type, p->allocator.memory_alloc));

        UNREACHABLE_IF_ERROR(parser_next_token(p));
        is_default_initialized = true;
    } else if (parser_peek_token_is(p, COLON)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
        PROPAGATE_IF_ERROR(explicit_type_parse(p, start_token, &type));

        if (parser_peek_token_is(p, ASSIGN)) {
            is_default_initialized = true;
            UNREACHABLE_IF_ERROR(parser_next_token(p));
        }
    } else {
        PROPAGATE_IF_ERROR_IS(parser_peek_error(p, COLON), REALLOCATION_FAILED);
        return UNEXPECTED_TOKEN;
    }

    // Try to advance again to prepare for rhs
    if (p->lexer_index < p->lexer->token_accumulator.length) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
    }

    *expression  = (Expression*)type;
    *initialized = is_default_initialized;
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
    const Token start_token = p->current_token;
    PROPAGATE_IF_ERROR(parser_expect_peek(p, LBRACE));

    ArrayList members;
    PROPAGATE_IF_ERROR(array_list_init_allocator(&members, 4, sizeof(StructMember), p->allocator));
    while (true) {
        PROPAGATE_IF_ERROR_DO(parser_expect_peek(p, IDENT),
                              free_struct_member_list(&members, p->allocator.free_alloc));

        Expression* ident_expr;
        PROPAGATE_IF_ERROR_DO(identifier_expression_parse(p, &ident_expr),
                              free_struct_member_list(&members, p->allocator.free_alloc));

        Expression* type_expr;
        bool        has_default_value;
        PROPAGATE_IF_ERROR_DO(type_expression_parse(p, &type_expr, &has_default_value), {
            identifier_expression_destroy((Node*)ident_expr, p->allocator.free_alloc);
            free_struct_member_list(&members, p->allocator.free_alloc);
        });

        // Struct members must have explicit type declarations
        TypeExpression* type = (TypeExpression*)type_expr;
        if (type->type.tag == IMPLICIT) {
            const Token error_token = p->current_token;
            IGNORE_STATUS(parser_put_status_error(
                p, STRUCT_MEMBER_NOT_EXPLICIT, error_token.line, error_token.column));

            identifier_expression_destroy((Node*)ident_expr, p->allocator.free_alloc);
            type_expression_destroy((Node*)type_expr, p->allocator.free_alloc);
            free_struct_member_list(&members, p->allocator.free_alloc);

            return STRUCT_MEMBER_NOT_EXPLICIT;
        }

        // The type parser leaves with the current token on assignment in this case
        Expression* default_value = NULL;
        if (has_default_value) {
            PROPAGATE_IF_ERROR_DO(expression_parse(p, LOWEST, &default_value), {
                identifier_expression_destroy((Node*)ident_expr, p->allocator.free_alloc);
                type_expression_destroy((Node*)type_expr, p->allocator.free_alloc);
                free_struct_member_list(&members, p->allocator.free_alloc);
            });
        }

        // Now we can create the member and store its data pointers
        const StructMember member = (StructMember){
            .name          = (IdentifierExpression*)ident_expr,
            .type          = type,
            .default_value = default_value,
        };

        PROPAGATE_IF_ERROR_DO(array_list_push(&members, &member), {
            identifier_expression_destroy((Node*)ident_expr, p->allocator.free_alloc);
            type_expression_destroy((Node*)type_expr, p->allocator.free_alloc);
            NODE_VIRTUAL_FREE(default_value, p->allocator.free_alloc);
            free_struct_member_list(&members, p->allocator.free_alloc);
        });

        // All members require a trailing comma!
        if (has_default_value && parser_peek_token_is(p, COMMA)) {
            UNREACHABLE_IF_ERROR(parser_next_token(p));
        } else if (!parser_current_token_is(p, COMMA)) {
            const Token error_token = p->current_token;
            IGNORE_STATUS(parser_put_status_error(
                p, MISSING_TRAILING_COMMA, error_token.line, error_token.column));
            free_struct_member_list(&members, p->allocator.free_alloc);
            return MISSING_TRAILING_COMMA;
        }

        // We only handle members here as methods are in impl blocks
        if (parser_peek_token_is(p, RBRACE)) {
            UNREACHABLE_IF_ERROR(parser_next_token(p));
            if (parser_peek_token_is(p, SEMICOLON)) {
                UNREACHABLE_IF_ERROR(parser_next_token(p));
            }

            break;
        }
    }

    StructExpression* struct_expr;
    PROPAGATE_IF_ERROR_DO(
        struct_expression_create(start_token, members, &struct_expr, p->allocator.memory_alloc),
        free_struct_member_list(&members, p->allocator.free_alloc));

    *expression = (Expression*)struct_expr;
    return SUCCESS;
}

TRY_STATUS enum_expression_parse(Parser* p, Expression** expression) {
    const Token start_token = p->current_token;
    PROPAGATE_IF_ERROR(parser_expect_peek(p, LBRACE));

    ArrayList variants;
    PROPAGATE_IF_ERROR(array_list_init_allocator(&variants, 4, sizeof(EnumVariant), p->allocator));
    while (true) {
        PROPAGATE_IF_ERROR_DO(parser_expect_peek(p, IDENT),
                              free_enum_variant_list(&variants, p->allocator.free_alloc));

        Expression* ident_expr;
        PROPAGATE_IF_ERROR_DO(identifier_expression_parse(p, &ident_expr),
                              free_enum_variant_list(&variants, p->allocator.free_alloc));
        IdentifierExpression* ident = (IdentifierExpression*)ident_expr;

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

        EnumVariant variant = (EnumVariant){.name = ident, .value = value};
        PROPAGATE_IF_ERROR_DO(array_list_push(&variants, &variant), {
            free_enum_variant_list(&variants, p->allocator.free_alloc);
            identifier_expression_destroy((Node*)ident, p->allocator.free_alloc);
            NODE_VIRTUAL_FREE(value, p->allocator.free_alloc);
        });

        // All variants require a trailing comma!
        if (STATUS_ERR(parser_expect_peek(p, COMMA))) {
            const Token error_token = p->current_token;
            IGNORE_STATUS(parser_put_status_error(
                p, MISSING_TRAILING_COMMA, error_token.line, error_token.column));
            free_enum_variant_list(&variants, p->allocator.free_alloc);
            return MISSING_TRAILING_COMMA;
        }

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

TRY_STATUS nil_expression_parse(Parser* p, Expression** expression) {
    NilExpression* nil;
    PROPAGATE_IF_ERROR(nil_expression_create(p->current_token, &nil, p->allocator.memory_alloc));
    *expression = (Expression*)nil;
    return SUCCESS;
}

TRY_STATUS continue_expression_parse(Parser* p, Expression** expression) {
    ContinueExpression* continue_expr;
    PROPAGATE_IF_ERROR(
        continue_expression_create(p->current_token, &continue_expr, p->allocator.memory_alloc));
    *expression = (Expression*)continue_expr;
    return SUCCESS;
}

TRY_STATUS match_expression_parse(Parser* p, Expression** expression) {
    const Token start_token = p->current_token;
    PROPAGATE_IF_ERROR(parser_next_token(p));

    Expression* match_cond_expr;
    PROPAGATE_IF_ERROR(expression_parse(p, LOWEST, &match_cond_expr));
    PROPAGATE_IF_ERROR_DO(parser_expect_peek(p, LBRACE),
                          NODE_VIRTUAL_FREE(match_cond_expr, p->allocator.free_alloc));

    ArrayList arms;
    PROPAGATE_IF_ERROR_DO(array_list_init_allocator(&arms, 4, sizeof(MatchArm), p->allocator),
                          NODE_VIRTUAL_FREE(match_cond_expr, p->allocator.free_alloc));
    while (!parser_peek_token_is(p, RBRACE) && !parser_peek_token_is(p, END)) {
        // Current token which is either the LBRACE at the start or a comma before parsing
        UNREACHABLE_IF_ERROR(parser_next_token(p));

        Expression* pattern;
        PROPAGATE_IF_ERROR_DO(expression_parse(p, LOWEST, &pattern),
                              NODE_VIRTUAL_FREE(match_cond_expr, p->allocator.free_alloc));
        PROPAGATE_IF_ERROR_DO(parser_expect_peek(p, FAT_ARROW), {
            NODE_VIRTUAL_FREE(match_cond_expr, p->allocator.free_alloc);
            NODE_VIRTUAL_FREE(pattern, p->allocator.free_alloc);
            free_match_arm_list(&arms, p->allocator.free_alloc);
        });

        // Guard the statement from being global/declaration based
        PROPAGATE_IF_ERROR_DO(parser_next_token(p), {
            NODE_VIRTUAL_FREE(match_cond_expr, p->allocator.free_alloc);
            NODE_VIRTUAL_FREE(pattern, p->allocator.free_alloc);
            free_match_arm_list(&arms, p->allocator.free_alloc);
        });

        Statement* consequence;
        switch (p->current_token.type) {
        case VAR:
        case CONST:
        case TYPE:
        case IMPL:
        case IMPORT:
            IGNORE_STATUS(parser_put_status_error(
                p, ILLEGAL_MATCH_ARM, p->current_token.line, p->current_token.column));

            NODE_VIRTUAL_FREE(match_cond_expr, p->allocator.free_alloc);
            NODE_VIRTUAL_FREE(pattern, p->allocator.free_alloc);
            free_match_arm_list(&arms, p->allocator.free_alloc);
            return ILLEGAL_MATCH_ARM;
        default:
            // The statement can either be a jump, expression, or block statement
            PROPAGATE_IF_ERROR_DO(parser_parse_statement(p, &consequence), {
                NODE_VIRTUAL_FREE(match_cond_expr, p->allocator.free_alloc);
                NODE_VIRTUAL_FREE(pattern, p->allocator.free_alloc);
                free_match_arm_list(&arms, p->allocator.free_alloc);
            });
            break;
        }

        // As per the rest of the language, commas are required as trailing tokens
        PROPAGATE_IF_ERROR_DO(parser_expect_peek(p, COMMA), {
            NODE_VIRTUAL_FREE(match_cond_expr, p->allocator.free_alloc);
            NODE_VIRTUAL_FREE(pattern, p->allocator.free_alloc);
            NODE_VIRTUAL_FREE(consequence, p->allocator.free_alloc);
            free_match_arm_list(&arms, p->allocator.free_alloc);
        });

        MatchArm arm = (MatchArm){.pattern = pattern, .dispatch = consequence};
        PROPAGATE_IF_ERROR_DO(array_list_push(&arms, &arm), {
            NODE_VIRTUAL_FREE(match_cond_expr, p->allocator.free_alloc);
            NODE_VIRTUAL_FREE(pattern, p->allocator.free_alloc);
            NODE_VIRTUAL_FREE(consequence, p->allocator.free_alloc);
            free_match_arm_list(&arms, p->allocator.free_alloc);
        });
    }

    // Empty match statements aren't ever allowed
    if (arms.length == 0) {
        IGNORE_STATUS(
            parser_put_status_error(p, ARMLESS_MATCH_EXPR, start_token.line, start_token.column));

        NODE_VIRTUAL_FREE(match_cond_expr, p->allocator.free_alloc);
        free_match_arm_list(&arms, p->allocator.free_alloc);
        return ARMLESS_MATCH_EXPR;
    }

    MatchExpression* match;
    PROPAGATE_IF_ERROR_DO(
        match_expression_create(
            start_token, match_cond_expr, arms, &match, p->allocator.memory_alloc),
        {
            NODE_VIRTUAL_FREE(match_cond_expr, p->allocator.free_alloc);
            free_match_arm_list(&arms, p->allocator.free_alloc);
        });

    PROPAGATE_IF_ERROR_DO(parser_expect_peek(p, RBRACE),
                          match_expression_destroy((Node*)match, p->allocator.free_alloc));
    if (parser_peek_token_is(p, SEMICOLON)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
    }

    *expression = (Expression*)match;
    return SUCCESS;
}
