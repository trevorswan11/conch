#include <assert.h>

#include "lexer/token.h"

#include "parser/expression_parsers.h"
#include "parser/parser.h"
#include "parser/precedence.h"
#include "parser/statement_parsers.h"

#include "ast/ast.h"
#include "ast/expressions/array.h"
#include "ast/expressions/assignment.h"
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
#include "ast/expressions/loop.h"
#include "ast/expressions/match.h"
#include "ast/expressions/narrow.h"
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

static inline NODISCARD Status record_missing_prefix(Parser* p) {
    const Token   current = p->current_token;
    StringBuilder sb;
    TRY(string_builder_init_allocator(&sb, 50, p->allocator));

    const char start[] = "No prefix parse function for ";
    TRY_DO(string_builder_append_many(&sb, start, sizeof(start) - 1), string_builder_deinit(&sb));

    const char* token_literal = token_type_name(current.type);
    TRY_DO(string_builder_append_str_z(&sb, token_literal), string_builder_deinit(&sb));

    const char end[] = " found";
    TRY_DO(string_builder_append_many(&sb, end, sizeof(end) - 1), string_builder_deinit(&sb));

    TRY_DO(error_append_ln_col(current.line, current.column, &sb), string_builder_deinit(&sb));

    MutSlice slice;
    TRY_DO(string_builder_to_string(&sb, &slice), string_builder_deinit(&sb));
    TRY_DO(array_list_push(&p->errors, &slice), string_builder_deinit(&sb));
    return SUCCESS;
}

NODISCARD Status expression_parse(Parser* p, Precedence precedence, Expression** lhs_expression) {
    assert(p);
    ASSERT_ALLOCATOR(p->allocator);

    PrefixFn prefix;
    if (!poll_prefix(p, p->current_token.type, &prefix)) {
        TRY(record_missing_prefix(p));
        return ELEMENT_MISSING;
    }
    TRY(prefix.prefix_parse(p, lhs_expression));

    while (!parser_peek_token_is(p, SEMICOLON) && precedence < parser_peek_precedence(p)) {
        InfixFn infix;
        if (!poll_infix(p, p->peek_token.type, &infix)) {
            return SUCCESS;
        }

        TRY(parser_next_token(p));
        TRY_DO(infix.infix_parse(p, *lhs_expression, lhs_expression),
               NODE_VIRTUAL_FREE(*lhs_expression, p->allocator.free_alloc));
    }

    return SUCCESS;
}

NODISCARD Status identifier_expression_parse(Parser* p, Expression** expression) {
    const Token start_token = p->current_token;
    if (start_token.type != IDENT && !hash_set_contains(&p->primitives, &start_token.type)) {
        IGNORE_STATUS(
            parser_put_status_error(p, ILLEGAL_IDENTIFIER, start_token.line, start_token.column));
        return ILLEGAL_IDENTIFIER;
    }

    char* mut_name = strdup_s_allocator(
        start_token.slice.ptr, start_token.slice.length, p->allocator.memory_alloc);
    if (!mut_name) {
        return ALLOCATION_FAILED;
    }
    MutSlice name = mut_slice_from_str_z(mut_name);

    IdentifierExpression* ident;
    TRY_DO(identifier_expression_create(start_token, name, &ident, p->allocator.memory_alloc),
           p->allocator.free_alloc(name.ptr));
    *expression = (Expression*)ident;
    return SUCCESS;
}

NODISCARD Status generics_parse(Parser* p, ArrayList* generics) {
    TRY(array_list_init_allocator(generics, 1, sizeof(Expression*), p->allocator));
    if (parser_peek_token_is(p, LT)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
        if (parser_peek_token_is(p, GT)) {
            IGNORE_STATUS(parser_put_status_error(
                p, EMPTY_GENERIC_LIST, p->current_token.line, p->current_token.column));
            free_expression_list(generics, p->allocator.free_alloc);
            return EMPTY_GENERIC_LIST;
        }

        while (!parser_peek_token_is(p, GT) && !parser_peek_token_is(p, END)) {
            UNREACHABLE_IF_ERROR(parser_next_token(p));

            Expression* ident;
            TRY_DO(identifier_expression_parse(p, &ident),
                   free_expression_list(generics, p->allocator.free_alloc));

            TRY_DO(array_list_push(generics, &ident), {
                free_expression_list(generics, p->allocator.free_alloc);
                NODE_VIRTUAL_FREE(ident, p->allocator.free_alloc);
            });

            if (!parser_peek_token_is(p, GT)) {
                TRY_DO(parser_expect_peek(p, COMMA),
                       free_expression_list(generics, p->allocator.free_alloc));
            }
        }

        TRY_DO(parser_expect_peek(p, GT), free_expression_list(generics, p->allocator.free_alloc));
    }

    return SUCCESS;
}

NODISCARD Status function_definition_parse(Parser*          p,
                                           ArrayList*       generics,
                                           ArrayList*       parameters,
                                           TypeExpression** return_type,
                                           bool*            contains_default_param) {
    assert(parser_current_token_is(p, FUNCTION));
    TRY(generics_parse(p, generics));

    TRY_DO(parser_expect_peek(p, LPAREN), free_expression_list(generics, p->allocator.free_alloc));
    TRY_DO(allocate_parameter_list(p, parameters, contains_default_param),
           free_expression_list(generics, p->allocator.free_alloc));

    TRY_DO(parser_expect_peek(p, COLON), {
        free_parameter_list(parameters, p->allocator.free_alloc);
        free_expression_list(generics, p->allocator.free_alloc);
    });
    const Token type_token_start = p->current_token;

    TRY_DO(explicit_type_parse(p, type_token_start, return_type), {
        IGNORE_STATUS(parser_put_status_error(
            p, MALFORMED_FUNCTION_LITERAL, type_token_start.line, type_token_start.column));
        free_parameter_list(parameters, p->allocator.free_alloc);
        free_expression_list(generics, p->allocator.free_alloc);
    });

    return SUCCESS;
}

NODISCARD Status explicit_type_parse(Parser* p, Token start_token, TypeExpression** type) {
    // Check for a question mark and to allow nil
    bool is_nullable = false;
    if (parser_peek_token_is(p, WHAT)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
        is_nullable = true;
    }

    // Arrays are a little weird especially with the function signature
    ArrayList dim_array;
    dim_array.data              = NULL;
    bool is_array_type          = false;
    bool is_inner_type_nullable = false;

    if (parser_peek_token_is(p, LBRACKET)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));

        TRY(array_list_init_allocator(&dim_array, 1, sizeof(uint64_t), p->allocator));

        while (!parser_peek_token_is(p, RBRACKET) && !parser_peek_token_is(p, END)) {
            UNREACHABLE_IF_ERROR(parser_next_token(p));
            const Token integer_token = p->current_token;

            if (!token_is_unsigned_integer(integer_token.type)) {
                IGNORE_STATUS(parser_put_status_error(
                    p, UNEXPECTED_ARRAY_SIZE_TOKEN, integer_token.line, integer_token.column));
                array_list_deinit(&dim_array);
                return UNEXPECTED_ARRAY_SIZE_TOKEN;
            }

            uint64_t dim;
            TRY_DO(strntoull(integer_token.slice.ptr,
                             integer_token.slice.length - 1,
                             integer_token_to_base(integer_token.type),
                             &dim),
                   array_list_deinit(&dim_array));

            TRY_DO(array_list_push(&dim_array, &dim), array_list_deinit(&dim_array));

            if (parser_peek_token_is(p, COMMA)) {
                UNREACHABLE_IF_ERROR(parser_next_token(p));
            }
        }

        TRY_DO(parser_expect_peek(p, RBRACKET), array_list_deinit(&dim_array));
        if (dim_array.length == 0) {
            array_list_deinit(&dim_array);
            IGNORE_STATUS(parser_put_status_error(
                p, MISSING_ARRAY_SIZE_TOKEN, start_token.line, start_token.column));
            return MISSING_ARRAY_SIZE_TOKEN;
        }
        is_array_type = true;

        if (parser_peek_token_is(p, WHAT)) {
            UNREACHABLE_IF_ERROR(parser_next_token(p));
            is_inner_type_nullable = true;
        }
    } else {
        // If we don;t have an array, the inner type is just the normal type and needs to be set
        is_inner_type_nullable = is_nullable;
    }

    // Check for primitive before parsing the real type
    const bool is_primitive   = hash_set_contains(&p->primitives, &p->peek_token.type);
    TypeUnion  explicit_union = (TypeUnion){
         .explicit_type =
            (ExplicitType){
                 .tag = EXPLICIT_IDENT,
                 .variant =
                    (ExplicitTypeUnion){
                         .ident_type_name = NULL,
                    },
                 .nullable  = is_inner_type_nullable,
                 .primitive = is_primitive,
            },
    };

    if (is_primitive || parser_peek_token_is(p, IDENT)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));

        Expression* ident_expr;
        TRY_DO(identifier_expression_parse(p, &ident_expr), array_list_deinit(&dim_array));
        IdentifierExpression* ident = (IdentifierExpression*)ident_expr;

        explicit_union.explicit_type.tag     = EXPLICIT_IDENT;
        explicit_union.explicit_type.variant = (ExplicitTypeUnion){
            .ident_type_name = ident,
        };

        TRY_DO(type_expression_create(
                   start_token, EXPLICIT, explicit_union, type, p->allocator.memory_alloc),
               {
                   identifier_expression_destroy((Node*)ident_expr, p->allocator.free_alloc);
                   array_list_deinit(&dim_array);
               });
    } else {
        const Token type_start = p->current_token;
        UNREACHABLE_IF_ERROR(parser_next_token(p));

        switch (p->current_token.type) {
        case FUNCTION: {
            bool            contains_default_param;
            ArrayList       generics;
            ArrayList       parameters;
            TypeExpression* return_type;
            TRY_DO(function_definition_parse(
                       p, &generics, &parameters, &return_type, &contains_default_param),
                   array_list_deinit(&dim_array));

            // Default values in function types make no sense to allow
            if (contains_default_param) {
                TRY_DO(parser_put_status_error(
                           p, MALFORMED_FUNCTION_LITERAL, type_start.line, type_start.column),
                       {
                           free_expression_list(&generics, p->allocator.free_alloc);
                           free_parameter_list(&parameters, p->allocator.free_alloc);
                           type_expression_destroy((Node*)return_type, p->allocator.free_alloc);
                           array_list_deinit(&dim_array);
                       });
            }

            explicit_union.explicit_type.tag     = EXPLICIT_FN;
            explicit_union.explicit_type.variant = (ExplicitTypeUnion){
                .function_type =
                    (ExplicitFunctionType){
                        .fn_generics    = generics,
                        .fn_type_params = parameters,
                        .return_type    = return_type,
                    },
            };

            TRY_DO(type_expression_create(
                       start_token, EXPLICIT, explicit_union, type, p->allocator.memory_alloc),
                   {
                       free_expression_list(&generics, p->allocator.free_alloc);
                       free_parameter_list(&parameters, p->allocator.free_alloc);
                       type_expression_destroy((Node*)return_type, p->allocator.free_alloc);
                       array_list_deinit(&dim_array);
                   });
            break;
        }
        case STRUCT: {
            Expression* struct_expr;
            TRY_DO(struct_expression_parse(p, &struct_expr), array_list_deinit(&dim_array));
            StructExpression* struct_type = (StructExpression*)struct_expr;

            explicit_union.explicit_type.tag     = EXPLICIT_STRUCT;
            explicit_union.explicit_type.variant = (ExplicitTypeUnion){
                .struct_type = struct_type,
            };

            TRY_DO(type_expression_create(
                       start_token, EXPLICIT, explicit_union, type, p->allocator.memory_alloc),
                   {
                       struct_expression_destroy((Node*)struct_type, p->allocator.free_alloc);
                       array_list_deinit(&dim_array);
                   });
            break;
        }
        case ENUM: {
            Expression* enum_expr;
            TRY_DO(enum_expression_parse(p, &enum_expr), array_list_deinit(&dim_array));
            EnumExpression* enum_type = (EnumExpression*)enum_expr;

            explicit_union.explicit_type.tag     = EXPLICIT_ENUM;
            explicit_union.explicit_type.variant = (ExplicitTypeUnion){
                .enum_type = enum_type,
            };

            TRY_DO(type_expression_create(
                       start_token, EXPLICIT, explicit_union, type, p->allocator.memory_alloc),
                   {
                       enum_expression_destroy((Node*)enum_type, p->allocator.free_alloc);
                       array_list_deinit(&dim_array);
                   });
            break;
        }
        default:
            array_list_deinit(&dim_array);
            return UNEXPECTED_TOKEN;
        }
    }

    // If we parsed an array type at the start, then the actual type is nested
    if (is_array_type) {
        TypeUnion array_type_union = (TypeUnion){
            .explicit_type =
                (ExplicitType){
                    .tag = EXPLICIT_ARRAY,
                    .variant =
                        (ExplicitTypeUnion){
                            .array_type =
                                (ExplicitArrayType){
                                    .inner_type = *type,
                                    .dimensions = dim_array,
                                },
                        },
                    .nullable  = is_nullable,
                    .primitive = false,
                },
        };

        // Reassign the pointer's pointer, but the actual memory is owned by the new union here
        TRY_DO(type_expression_create(
                   start_token, EXPLICIT, array_type_union, type, p->allocator.memory_alloc),
               {
                   array_list_deinit(&dim_array);
                   type_expression_destroy((Node*)*type, p->allocator.free_alloc);
               });
    }

    return SUCCESS;
}

NODISCARD Status type_expression_parse(Parser* p, Expression** expression, bool* initialized) {
    const Token start_token = p->current_token;
    assert(p->primitives.buffer);
    bool is_default_initialized = false;

    TypeExpression* type;
    if (parser_peek_token_is(p, WALRUS)) {
        TRY(type_expression_create(
            start_token, IMPLICIT, IMPLICIT_TYPE, &type, p->allocator.memory_alloc));

        UNREACHABLE_IF_ERROR(parser_next_token(p));
        is_default_initialized = true;
    } else if (parser_peek_token_is(p, COLON)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
        TRY(explicit_type_parse(p, start_token, &type));

        if (parser_peek_token_is(p, ASSIGN)) {
            is_default_initialized = true;
            UNREACHABLE_IF_ERROR(parser_next_token(p));
        }
    } else {
        TRY_IS(parser_peek_error(p, COLON), REALLOCATION_FAILED);
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

NODISCARD Status integer_literal_expression_parse(Parser* p, Expression** expression) {
    const Token start_token = p->current_token;
    assert(start_token.slice.length > 0);
    const int base = integer_token_to_base(start_token.type);

    if (token_is_signed_integer(start_token.type)) {
        int64_t      value;
        const Status parse_status =
            strntoll(start_token.slice.ptr, start_token.slice.length, base, &value);
        if (STATUS_ERR(parse_status)) {
            TRY(parser_put_status_error(p, parse_status, start_token.line, start_token.column));
            return parse_status;
        }

        IntegerLiteralExpression* integer;
        TRY(integer_literal_expression_create(
            start_token, value, &integer, p->allocator.memory_alloc));

        *expression = (Expression*)integer;
    } else if (token_is_unsigned_integer(start_token.type)) {
        uint64_t     value;
        const Status parse_status =
            strntoull(start_token.slice.ptr, start_token.slice.length - 1, base, &value);
        if (STATUS_ERR(parse_status)) {
            TRY(parser_put_status_error(p, parse_status, start_token.line, start_token.column));
            return parse_status;
        }

        UnsignedIntegerLiteralExpression* integer;
        TRY(uinteger_literal_expression_create(
            start_token, value, &integer, p->allocator.memory_alloc));

        *expression = (Expression*)integer;
    } else {
        return UNEXPECTED_TOKEN;
    }

    return SUCCESS;
}

NODISCARD Status byte_literal_expression_parse(Parser* p, Expression** expression) {
    const Token start_token = p->current_token;
    uint8_t     value;
    TRY(strntochr(start_token.slice.ptr, start_token.slice.length, &value));

    ByteLiteralExpression* byte;
    TRY(byte_literal_expression_create(start_token, value, &byte, p->allocator.memory_alloc));

    *expression = (Expression*)byte;
    return SUCCESS;
}

NODISCARD Status float_literal_expression_parse(Parser* p, Expression** expression) {
    const Token start_token = p->current_token;
    assert(start_token.slice.length > 0);

    double       value;
    const Status parse_status = strntod(start_token.slice.ptr,
                                        start_token.slice.length,
                                        &value,
                                        p->allocator.memory_alloc,
                                        p->allocator.free_alloc);
    if (STATUS_ERR(parse_status)) {
        TRY(parser_put_status_error(p, parse_status, start_token.line, start_token.column));
        return parse_status;
    }

    FloatLiteralExpression* float_expr;
    TRY(float_literal_expression_create(
        start_token, value, &float_expr, p->allocator.memory_alloc));

    *expression = (Expression*)float_expr;
    return SUCCESS;
}

NODISCARD Status prefix_expression_parse(Parser* p, Expression** expression) {
    const Token prefix_token = p->current_token;
    assert(prefix_token.slice.length > 0);
    TRY(parser_next_token(p));

    Expression* rhs;
    TRY(expression_parse(p, PREFIX, &rhs));

    PrefixExpression* prefix;
    TRY_DO(prefix_expression_create(prefix_token, rhs, &prefix, p->allocator.memory_alloc),
           NODE_VIRTUAL_FREE(rhs, p->allocator.free_alloc));

    *expression = (Expression*)prefix;
    return SUCCESS;
}

NODISCARD Status infix_expression_parse(Parser* p, Expression* left, Expression** expression) {
    const TokenType  op_token_type      = p->current_token.type;
    const Precedence current_precedence = parser_current_precedence(p);
    TRY(parser_next_token(p));
    ASSERT_EXPRESSION(left);

    Expression* right;
    TRY(expression_parse(p, current_precedence, &right));

    Node*            left_node = (Node*)left;
    InfixExpression* infix;
    TRY_DO(
        infix_expression_create(
            left_node->start_token, left, op_token_type, right, &infix, p->allocator.memory_alloc),
        NODE_VIRTUAL_FREE(right, p->allocator.free_alloc));

    *expression = (Expression*)infix;
    return SUCCESS;
}

NODISCARD Status bool_expression_parse(Parser* p, Expression** expression) {
    BoolLiteralExpression* boolean;
    TRY(bool_literal_expression_create(p->current_token, &boolean, p->allocator.memory_alloc));
    *expression = (Expression*)boolean;
    return SUCCESS;
}

NODISCARD Status string_expression_parse(Parser* p, Expression** expression) {
    StringLiteralExpression* string;
    TRY(string_literal_expression_create(p->current_token, &string, p->allocator));
    *expression = (Expression*)string;
    return SUCCESS;
}

NODISCARD Status grouped_expression_parse(Parser* p, Expression** expression) {
    UNREACHABLE_IF_ERROR(parser_next_token(p));

    Expression* inner;
    TRY(expression_parse(p, LOWEST, &inner));

    TRY_DO(parser_expect_peek(p, RPAREN), NODE_VIRTUAL_FREE(inner, p->allocator.free_alloc));

    *expression = inner;
    return SUCCESS;
}

static inline NODISCARD Status _if_expression_parse_branch(Parser* p, Statement** stmt) {
    TRY(parser_next_token(p));

    Statement* alternate;
    switch (p->current_token.type) {
    case VAR:
    case CONST:
    case TYPE:
    case IMPL:
    case IMPORT:
    case UNDERSCORE:
        IGNORE_STATUS(parser_put_status_error(
            p, ILLEGAL_IF_BRANCH, p->current_token.line, p->current_token.column));
        return ILLEGAL_IF_BRANCH;
    default:
        TRY(parser_parse_statement(p, &alternate));
        break;
    }

    *stmt = alternate;
    return SUCCESS;
}

NODISCARD Status if_expression_parse(Parser* p, Expression** expression) {
    const Token start_token = p->current_token;
    TRY(parser_expect_peek(p, LPAREN));
    TRY(parser_next_token(p));

    Expression* condition;
    TRY(expression_parse(p, LOWEST, &condition));

    TRY_DO(parser_expect_peek(p, RPAREN), NODE_VIRTUAL_FREE(condition, p->allocator.free_alloc));

    Statement* consequence;
    TRY_DO(_if_expression_parse_branch(p, &consequence),
           NODE_VIRTUAL_FREE(condition, p->allocator.free_alloc));

    Statement* alternate = NULL;
    if (parser_peek_token_is(p, ELSE)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
        TRY_DO(_if_expression_parse_branch(p, &alternate), {
            NODE_VIRTUAL_FREE(condition, p->allocator.free_alloc);
            NODE_VIRTUAL_FREE(consequence, p->allocator.free_alloc);
        });
    }

    IfExpression* if_expr;
    TRY_DO(if_expression_create(
               start_token, condition, consequence, alternate, &if_expr, p->allocator.memory_alloc),
           {
               NODE_VIRTUAL_FREE(condition, p->allocator.free_alloc);
               NODE_VIRTUAL_FREE(consequence, p->allocator.free_alloc);
               NODE_VIRTUAL_FREE(alternate, p->allocator.free_alloc);
           });

    *expression = (Expression*)if_expr;
    return SUCCESS;
}

NODISCARD Status function_expression_parse(Parser* p, Expression** expression) {
    const Token     start_token = p->current_token;
    ArrayList       generics;
    ArrayList       parameters;
    TypeExpression* return_type;

    TRY(function_definition_parse(p, &generics, &parameters, &return_type, NULL));
    TRY_DO(parser_expect_peek(p, LBRACE), {
        free_expression_list(&generics, p->allocator.free_alloc);
        free_parameter_list(&parameters, p->allocator.free_alloc);
        type_expression_destroy((Node*)return_type, p->allocator.free_alloc);
    });

    BlockStatement* body;
    TRY_DO(block_statement_parse(p, &body), {
        free_expression_list(&generics, p->allocator.free_alloc);
        free_parameter_list(&parameters, p->allocator.free_alloc);
        type_expression_destroy((Node*)return_type, p->allocator.free_alloc);
    });

    FunctionExpression* function;
    TRY_DO(function_expression_create(start_token,
                                      generics,
                                      parameters,
                                      return_type,
                                      body,
                                      &function,
                                      p->allocator.memory_alloc),
           {
               free_expression_list(&generics, p->allocator.free_alloc);
               type_expression_destroy((Node*)return_type, p->allocator.free_alloc);
               free_parameter_list(&parameters, p->allocator.free_alloc);
               block_statement_destroy((Node*)body, p->allocator.free_alloc);
           });

    *expression = (Expression*)function;
    return SUCCESS;
}

NODISCARD Status call_expression_parse(Parser* p, Expression* function, Expression** expression) {
    const Token start_token = p->current_token;

    ArrayList arguments;
    TRY(array_list_init_allocator(&arguments, 2, sizeof(CallArgument), p->allocator));

    if (parser_peek_token_is(p, RPAREN)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
    } else {
        bool is_ref_argument = false;
        if (parser_peek_token_is(p, REF)) {
            UNREACHABLE_IF_ERROR(parser_next_token(p));
            is_ref_argument = true;
        }

        TRY_DO(parser_next_token(p), array_list_deinit(&arguments));

        // Append the first argument to prepare for comma peeking
        Expression* argument_expr;
        TRY_DO(expression_parse(p, LOWEST, &argument_expr), array_list_deinit(&arguments));

        CallArgument argument =
            (CallArgument){.is_ref = is_ref_argument, .argument = argument_expr};
        TRY_DO(array_list_push(&arguments, &argument), {
            NODE_VIRTUAL_FREE(argument_expr, p->allocator.free_alloc);
            array_list_deinit(&arguments);
        });

        // Add the rest of the comma separated argument list
        while (parser_peek_token_is(p, COMMA)) {
            is_ref_argument = false;
            UNREACHABLE_IF_ERROR(parser_next_token(p));
            if (parser_peek_token_is(p, REF)) {
                UNREACHABLE_IF_ERROR(parser_next_token(p));
                is_ref_argument = true;
            }

            TRY_DO(parser_next_token(p),
                   free_call_expression_list(&arguments, p->allocator.free_alloc));

            TRY_DO(expression_parse(p, LOWEST, &argument_expr),
                   free_call_expression_list(&arguments, p->allocator.free_alloc));

            argument = (CallArgument){.is_ref = is_ref_argument, .argument = argument_expr};
            TRY_DO(array_list_push(&arguments, &argument),
                   free_call_expression_list(&arguments, p->allocator.free_alloc));
        }

        TRY_DO(parser_expect_peek(p, RPAREN),
               free_call_expression_list(&arguments, p->allocator.free_alloc));
    }

    // Since call expressions are infixed, their generics are at the end of the expression
    ArrayList generics;
    if (parser_peek_token_is(p, WITH)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
        TRY_DO(generics_parse(p, &generics),
               free_call_expression_list(&arguments, p->allocator.free_alloc));
    } else {
        TRY(array_list_init_allocator(&generics, 1, sizeof(Expression*), p->allocator));
    }

    // We can just move into the call expression now since generics is necessary allocated
    CallExpression* call;
    TRY_DO(call_expression_create(
               start_token, function, arguments, generics, &call, p->allocator.memory_alloc),
           {
               free_call_expression_list(&arguments, p->allocator.free_alloc);
               free_expression_list(&generics, p->allocator.free_alloc);
           });

    *expression = (Expression*)call;
    return SUCCESS;
}

NODISCARD Status struct_expression_parse(Parser* p, Expression** expression) {
    const Token start_token = p->current_token;
    ArrayList   generics;
    TRY(generics_parse(p, &generics));

    TRY_DO(parser_expect_peek(p, LBRACE), free_expression_list(&generics, p->allocator.free_alloc));
    if (parser_peek_token_is(p, RBRACE)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
        IGNORE_STATUS(parser_put_status_error(
            p, STRUCT_MISSING_MEMBERS, start_token.line, start_token.column));
        free_expression_list(&generics, p->allocator.free_alloc);
        return STRUCT_MISSING_MEMBERS;
    }

    ArrayList members;
    TRY_DO(array_list_init_allocator(&members, 4, sizeof(StructMember), p->allocator),
           free_expression_list(&generics, p->allocator.free_alloc));

    // We only handle members here as methods are in impl blocks
    while (!parser_peek_token_is(p, RBRACE) && !parser_peek_token_is(p, END)) {
        TRY_DO(parser_expect_peek(p, IDENT), {
            free_struct_member_list(&members, p->allocator.free_alloc);
            free_expression_list(&generics, p->allocator.free_alloc);
        });

        Expression* ident_expr;
        TRY_DO(identifier_expression_parse(p, &ident_expr), {
            free_struct_member_list(&members, p->allocator.free_alloc);
            free_expression_list(&generics, p->allocator.free_alloc);
        });

        Expression* type_expr;
        bool        has_default_value;
        TRY_DO(type_expression_parse(p, &type_expr, &has_default_value), {
            identifier_expression_destroy((Node*)ident_expr, p->allocator.free_alloc);
            free_struct_member_list(&members, p->allocator.free_alloc);
            free_expression_list(&generics, p->allocator.free_alloc);
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
            free_expression_list(&generics, p->allocator.free_alloc);

            return STRUCT_MEMBER_NOT_EXPLICIT;
        }

        // The type parser leaves with the current token on assignment in this case
        Expression* default_value = NULL;
        if (has_default_value) {
            TRY_DO(expression_parse(p, LOWEST, &default_value), {
                identifier_expression_destroy((Node*)ident_expr, p->allocator.free_alloc);
                type_expression_destroy((Node*)type_expr, p->allocator.free_alloc);
                free_struct_member_list(&members, p->allocator.free_alloc);
                free_expression_list(&generics, p->allocator.free_alloc);
            });
        }

        // Now we can create the member and store its data pointers
        const StructMember member = (StructMember){
            .name          = (IdentifierExpression*)ident_expr,
            .type          = type,
            .default_value = default_value,
        };

        TRY_DO(array_list_push(&members, &member), {
            identifier_expression_destroy((Node*)ident_expr, p->allocator.free_alloc);
            type_expression_destroy((Node*)type_expr, p->allocator.free_alloc);
            NODE_VIRTUAL_FREE(default_value, p->allocator.free_alloc);
            free_struct_member_list(&members, p->allocator.free_alloc);
            free_expression_list(&generics, p->allocator.free_alloc);
        });

        // All members require a trailing comma!
        if (has_default_value && parser_peek_token_is(p, COMMA)) {
            UNREACHABLE_IF_ERROR(parser_next_token(p));
        } else if (!parser_current_token_is(p, COMMA)) {
            const Token error_token = p->current_token;
            IGNORE_STATUS(parser_put_status_error(
                p, MISSING_TRAILING_COMMA, error_token.line, error_token.column));
            free_struct_member_list(&members, p->allocator.free_alloc);
            free_expression_list(&generics, p->allocator.free_alloc);

            return MISSING_TRAILING_COMMA;
        }
    }

    StructExpression* struct_expr;
    TRY_DO(struct_expression_create(
               start_token, generics, members, &struct_expr, p->allocator.memory_alloc),
           {
               free_struct_member_list(&members, p->allocator.free_alloc);
               free_expression_list(&generics, p->allocator.free_alloc);
           });

    TRY_DO(parser_expect_peek(p, RBRACE),
           struct_expression_destroy((Node*)struct_expr, p->allocator.free_alloc));
    if (parser_peek_token_is(p, SEMICOLON)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
    }

    *expression = (Expression*)struct_expr;
    return SUCCESS;
}

NODISCARD Status enum_expression_parse(Parser* p, Expression** expression) {
    const Token start_token = p->current_token;
    TRY(parser_expect_peek(p, LBRACE));
    if (parser_peek_token_is(p, RBRACE)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
        IGNORE_STATUS(parser_put_status_error(
            p, ENUM_MISSING_VARIANTS, start_token.line, start_token.column));
        return ENUM_MISSING_VARIANTS;
    }

    ArrayList variants;
    TRY(array_list_init_allocator(&variants, 4, sizeof(EnumVariant), p->allocator));
    while (!parser_peek_token_is(p, RBRACE) && !parser_peek_token_is(p, END)) {
        TRY_DO(parser_expect_peek(p, IDENT),
               free_enum_variant_list(&variants, p->allocator.free_alloc));

        Expression* ident_expr;
        TRY_DO(identifier_expression_parse(p, &ident_expr),
               free_enum_variant_list(&variants, p->allocator.free_alloc));
        IdentifierExpression* ident = (IdentifierExpression*)ident_expr;

        Expression* value = NULL;
        if (parser_peek_token_is(p, ASSIGN)) {
            UNREACHABLE_IF_ERROR(parser_next_token(p));
            TRY_DO(parser_next_token(p), {
                free_enum_variant_list(&variants, p->allocator.free_alloc);
                identifier_expression_destroy((Node*)ident, p->allocator.free_alloc);
            });

            TRY_DO(expression_parse(p, LOWEST, &value), {
                free_enum_variant_list(&variants, p->allocator.free_alloc);
                identifier_expression_destroy((Node*)ident, p->allocator.free_alloc);
            });
        }

        EnumVariant variant = (EnumVariant){.name = ident, .value = value};
        TRY_DO(array_list_push(&variants, &variant), {
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
    }

    EnumExpression* enum_expr;
    TRY_DO(enum_expression_create(start_token, variants, &enum_expr, p->allocator.memory_alloc),
           free_enum_variant_list(&variants, p->allocator.free_alloc));

    TRY_DO(parser_expect_peek(p, RBRACE),
           enum_expression_destroy((Node*)enum_expr, p->allocator.free_alloc));
    if (parser_peek_token_is(p, SEMICOLON)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
    }

    *expression = (Expression*)enum_expr;
    return SUCCESS;
}

NODISCARD Status nil_expression_parse(Parser* p, Expression** expression) {
    NilExpression* nil;
    TRY(nil_expression_create(p->current_token, &nil, p->allocator.memory_alloc));
    *expression = (Expression*)nil;
    return SUCCESS;
}

NODISCARD Status match_expression_parse(Parser* p, Expression** expression) {
    const Token start_token = p->current_token;
    TRY(parser_next_token(p));

    Expression* match_cond_expr;
    TRY(expression_parse(p, LOWEST, &match_cond_expr));
    TRY_DO(parser_expect_peek(p, LBRACE),
           NODE_VIRTUAL_FREE(match_cond_expr, p->allocator.free_alloc));

    ArrayList arms;
    TRY_DO(array_list_init_allocator(&arms, 4, sizeof(MatchArm), p->allocator),
           NODE_VIRTUAL_FREE(match_cond_expr, p->allocator.free_alloc));
    while (!parser_peek_token_is(p, RBRACE) && !parser_peek_token_is(p, END)) {
        // Current token which is either the LBRACE at the start or a comma before parsing
        UNREACHABLE_IF_ERROR(parser_next_token(p));

        Expression* pattern;
        TRY_DO(expression_parse(p, LOWEST, &pattern),
               NODE_VIRTUAL_FREE(match_cond_expr, p->allocator.free_alloc));
        TRY_DO(parser_expect_peek(p, FAT_ARROW), {
            NODE_VIRTUAL_FREE(match_cond_expr, p->allocator.free_alloc);
            NODE_VIRTUAL_FREE(pattern, p->allocator.free_alloc);
            free_match_arm_list(&arms, p->allocator.free_alloc);
        });

        // Guard the statement from being global/declaration based
        TRY_DO(parser_next_token(p), {
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
        case UNDERSCORE:
            IGNORE_STATUS(parser_put_status_error(
                p, ILLEGAL_MATCH_ARM, p->current_token.line, p->current_token.column));

            NODE_VIRTUAL_FREE(match_cond_expr, p->allocator.free_alloc);
            NODE_VIRTUAL_FREE(pattern, p->allocator.free_alloc);
            free_match_arm_list(&arms, p->allocator.free_alloc);
            return ILLEGAL_MATCH_ARM;
        default:
            // The statement can either be a jump, expression, or block statement
            TRY_DO(parser_parse_statement(p, &consequence), {
                NODE_VIRTUAL_FREE(match_cond_expr, p->allocator.free_alloc);
                NODE_VIRTUAL_FREE(pattern, p->allocator.free_alloc);
                free_match_arm_list(&arms, p->allocator.free_alloc);
            });
            break;
        }

        // As per the rest of the language, commas are required as trailing tokens
        TRY_DO(parser_expect_peek(p, COMMA), {
            NODE_VIRTUAL_FREE(match_cond_expr, p->allocator.free_alloc);
            NODE_VIRTUAL_FREE(pattern, p->allocator.free_alloc);
            NODE_VIRTUAL_FREE(consequence, p->allocator.free_alloc);
            free_match_arm_list(&arms, p->allocator.free_alloc);
        });

        MatchArm arm = (MatchArm){.pattern = pattern, .dispatch = consequence};
        TRY_DO(array_list_push(&arms, &arm), {
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
    TRY_DO(match_expression_create(
               start_token, match_cond_expr, arms, &match, p->allocator.memory_alloc),
           {
               NODE_VIRTUAL_FREE(match_cond_expr, p->allocator.free_alloc);
               free_match_arm_list(&arms, p->allocator.free_alloc);
           });

    TRY_DO(parser_expect_peek(p, RBRACE),
           match_expression_destroy((Node*)match, p->allocator.free_alloc));
    if (parser_peek_token_is(p, SEMICOLON)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
    }

    *expression = (Expression*)match;
    return SUCCESS;
}

NODISCARD Status array_literal_expression_parse(Parser* p, Expression** expression) {
    const Token start_token = p->current_token;
    TRY(parser_next_token(p));

    uint64_t array_size;
    if (p->current_token.type == UNDERSCORE) {
        array_size = 0;
    } else {
        const Token integer_token = p->current_token;

        // Before parsing, check if we even have a type here
        if (parser_current_token_is(p, RBRACKET)) {
            IGNORE_STATUS(parser_put_status_error(
                p, MISSING_ARRAY_SIZE_TOKEN, integer_token.line, integer_token.column));
            return MISSING_ARRAY_SIZE_TOKEN;
        }

        if (!token_is_unsigned_integer(integer_token.type)) {
            TRY(parser_put_status_error(
                p, UNEXPECTED_ARRAY_SIZE_TOKEN, integer_token.line, integer_token.column));
            return UNEXPECTED_ARRAY_SIZE_TOKEN;
        }

        TRY(strntoull(integer_token.slice.ptr,
                      integer_token.slice.length - 1,
                      integer_token_to_base(integer_token.type),
                      &array_size));
    }

    TRY(parser_expect_peek(p, RBRACKET));
    TRY(parser_expect_peek(p, LBRACE));

    ArrayList items;
    TRY(array_list_init_allocator(
        &items, array_size == 0 ? 4 : array_size, sizeof(Expression*), p->allocator));

    while (!parser_peek_token_is(p, RBRACE) && !parser_peek_token_is(p, END)) {
        // Current token which is either the LBRACE at the start or a comma before parsing
        UNREACHABLE_IF_ERROR(parser_next_token(p));

        Expression* item;
        TRY_DO(expression_parse(p, LOWEST, &item),
               free_expression_list(&items, p->allocator.free_alloc));

        // Add to the array list and expect a comma to align with language philosophy
        TRY_DO(array_list_push(&items, &item), {
            NODE_VIRTUAL_FREE(item, p->allocator.free_alloc);
            free_expression_list(&items, p->allocator.free_alloc);
        });

        TRY_DO(parser_expect_peek(p, COMMA), free_expression_list(&items, p->allocator.free_alloc));
    }

    const bool inferred_size = array_size == 0;
    if (!inferred_size && items.length != array_size) {
        IGNORE_STATUS(parser_put_status_error(
            p, INCORRECT_EXPLICIT_ARRAY_SIZE, start_token.line, start_token.column));

        free_expression_list(&items, p->allocator.free_alloc);
        return INCORRECT_EXPLICIT_ARRAY_SIZE;
    }

    ArrayLiteralExpression* array;
    TRY_DO(array_literal_expression_create(
               start_token, inferred_size, items, &array, p->allocator.memory_alloc),
           free_expression_list(&items, p->allocator.free_alloc));

    TRY_DO(parser_expect_peek(p, RBRACE),
           array_literal_expression_destroy((Node*)array, p->allocator.free_alloc));
    if (parser_peek_token_is(p, SEMICOLON)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
    }

    *expression = (Expression*)array;
    return SUCCESS;
}

NODISCARD Status for_loop_expression_parse(Parser* p, Expression** expression) {
    const Token start_token = p->current_token;
    TRY(parser_expect_peek(p, LPAREN));

    ArrayList iterables;
    TRY(array_list_init_allocator(&iterables, 2, sizeof(Expression*), p->allocator));
    while (!parser_peek_token_is(p, RPAREN) && !parser_peek_token_is(p, END)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));

        Expression* iterable;
        TRY_DO(expression_parse(p, LOWEST, &iterable),
               free_expression_list(&iterables, p->allocator.free_alloc));

        TRY_DO(array_list_push(&iterables, &iterable),
               free_expression_list(&iterables, p->allocator.free_alloc));

        // Only check for the comma after confirming we aren't at the end of the iterables
        if (!parser_peek_token_is(p, RPAREN)) {
            TRY_DO(parser_expect_peek(p, COMMA),
                   free_expression_list(&iterables, p->allocator.free_alloc));
        }
    }

    TRY_DO(parser_expect_peek(p, RPAREN),
           free_expression_list(&iterables, p->allocator.free_alloc));

    if (iterables.length == 0) {
        IGNORE_STATUS(parser_put_status_error(
            p, FOR_MISSING_ITERABLES, start_token.line, start_token.column));

        free_expression_list(&iterables, p->allocator.free_alloc);
        return FOR_MISSING_ITERABLES;
    }

    // Captures are technically optional, but they are allocated anyways
    ArrayList captures;
    TRY(array_list_init_allocator(&captures, 2, sizeof(ForLoopCapture), p->allocator));

    bool expect_captures = false;
    if (parser_peek_token_is(p, COLON)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
        TRY_DO(parser_expect_peek(p, LPAREN), {
            free_expression_list(&iterables, p->allocator.free_alloc);
            free_for_capture_list(&captures, p->allocator.free_alloc);
        });

        while (!parser_peek_token_is(p, RPAREN) && !parser_peek_token_is(p, END)) {
            bool is_ref_argument = false;
            if (parser_peek_token_is(p, REF)) {
                UNREACHABLE_IF_ERROR(parser_next_token(p));
                is_ref_argument = true;
            }
            UNREACHABLE_IF_ERROR(parser_next_token(p));

            ForLoopCapture capture;
            if (parser_current_token_is(p, UNDERSCORE)) {
                IgnoreExpression* ignore;
                TRY_DO(
                    ignore_expression_create(p->current_token, &ignore, p->allocator.memory_alloc),
                    {
                        free_expression_list(&iterables, p->allocator.free_alloc);
                        free_for_capture_list(&captures, p->allocator.free_alloc);
                    });
                capture = (ForLoopCapture){.is_ref = false, .capture = (Expression*)ignore};
            } else {
                Expression* capture_expr;
                TRY_DO(expression_parse(p, LOWEST, &capture_expr), {
                    free_expression_list(&iterables, p->allocator.free_alloc);
                    free_for_capture_list(&captures, p->allocator.free_alloc);
                });
                capture = (ForLoopCapture){.is_ref = is_ref_argument, .capture = capture_expr};
            }

            TRY_DO(array_list_push(&captures, &capture), {
                free_expression_list(&iterables, p->allocator.free_alloc);
                free_for_capture_list(&captures, p->allocator.free_alloc);
            });

            // Only check for the close bar if we have more captures
            if (!parser_peek_token_is(p, RPAREN)) {
                TRY_DO(parser_expect_peek(p, COMMA), {
                    free_expression_list(&iterables, p->allocator.free_alloc);
                    free_for_capture_list(&captures, p->allocator.free_alloc);
                });
            }
        }

        TRY_DO(parser_expect_peek(p, RPAREN), {
            free_expression_list(&iterables, p->allocator.free_alloc);
            free_for_capture_list(&captures, p->allocator.free_alloc);
        });

        expect_captures = true;
    }

    // The number of captures must align with the number of iterables
    if (expect_captures && iterables.length != captures.length) {
        IGNORE_STATUS(parser_put_status_error(
            p, FOR_ITERABLE_CAPTURE_MISMATCH, start_token.line, start_token.column));

        free_expression_list(&iterables, p->allocator.free_alloc);
        free_for_capture_list(&captures, p->allocator.free_alloc);
        return FOR_ITERABLE_CAPTURE_MISMATCH;
    }

    // Now we can parse the block statement as usual
    TRY_DO(parser_expect_peek(p, LBRACE), {
        free_expression_list(&iterables, p->allocator.free_alloc);
        free_for_capture_list(&captures, p->allocator.free_alloc);
    });
    BlockStatement* block;
    TRY_DO(block_statement_parse(p, &block), {
        free_expression_list(&iterables, p->allocator.free_alloc);
        free_for_capture_list(&captures, p->allocator.free_alloc);
    });

    // There is nothing that can be done in an empty for loop so we prevent it here
    if (block->statements.length == 0) {
        IGNORE_STATUS(
            parser_put_status_error(p, EMPTY_FOR_LOOP, start_token.line, start_token.column));

        free_expression_list(&iterables, p->allocator.free_alloc);
        free_for_capture_list(&captures, p->allocator.free_alloc);
        block_statement_destroy((Node*)block, p->allocator.free_alloc);
        return EMPTY_FOR_LOOP;
    }

    // Finally, we should try to parse the non break clause
    Statement* non_break = NULL;
    if (parser_peek_token_is(p, ELSE)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
        TRY_DO(parser_next_token(p), {
            free_expression_list(&iterables, p->allocator.free_alloc);
            free_for_capture_list(&captures, p->allocator.free_alloc);
            block_statement_destroy((Node*)block, p->allocator.free_alloc);
        });

        // This logic follows the same restrictions as if branches and match arms
        switch (p->current_token.type) {
        case VAR:
        case CONST:
        case TYPE:
        case IMPL:
        case IMPORT:
        case UNDERSCORE:
            IGNORE_STATUS(parser_put_status_error(
                p, ILLEGAL_LOOP_NON_BREAK, p->current_token.line, p->current_token.column));

            free_expression_list(&iterables, p->allocator.free_alloc);
            free_for_capture_list(&captures, p->allocator.free_alloc);
            block_statement_destroy((Node*)block, p->allocator.free_alloc);
            return ILLEGAL_LOOP_NON_BREAK;
        default:
            TRY_DO(parser_parse_statement(p, &non_break), {
                free_expression_list(&iterables, p->allocator.free_alloc);
                free_for_capture_list(&captures, p->allocator.free_alloc);
                block_statement_destroy((Node*)block, p->allocator.free_alloc);
            });
            break;
        }
    }

    // We're now ready to construct the actual object and transfer ownership
    ForLoopExpression* for_loop;
    TRY_DO(for_loop_expression_create(start_token,
                                      iterables,
                                      captures,
                                      block,
                                      non_break,
                                      &for_loop,
                                      p->allocator.memory_alloc),
           {
               free_expression_list(&iterables, p->allocator.free_alloc);
               free_for_capture_list(&captures, p->allocator.free_alloc);
               block_statement_destroy((Node*)block, p->allocator.free_alloc);
               NODE_VIRTUAL_FREE(non_break, p->allocator.free_alloc);
           });

    if (parser_peek_token_is(p, SEMICOLON)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
    }

    *expression = (Expression*)for_loop;
    return SUCCESS;
}

NODISCARD Status while_loop_expression_parse(Parser* p, Expression** expression) {
    const Token start_token = p->current_token;
    TRY(parser_expect_peek(p, LPAREN));
    TRY(parser_next_token(p));

    Expression* condition;
    if (parser_current_token_is(p, RPAREN)) {
        IGNORE_STATUS(parser_put_status_error(
            p, WHILE_MISSING_CONDITION, p->current_token.line, p->current_token.column));
        return WHILE_MISSING_CONDITION;
    }
    TRY(expression_parse(p, LOWEST, &condition));

    TRY_DO(parser_expect_peek(p, RPAREN), NODE_VIRTUAL_FREE(condition, p->allocator.free_alloc));

    // The continuation expression is optional but is parsed similar to captures
    Expression* continuation = NULL;
    if (parser_peek_token_is(p, COLON)) {
        const Token continuation_start = p->current_token;
        UNREACHABLE_IF_ERROR(parser_next_token(p));
        TRY_DO(parser_expect_peek(p, LPAREN),
               NODE_VIRTUAL_FREE(condition, p->allocator.free_alloc));

        // We have to consume again since we're looking at the LPAREN here
        TRY_DO(parser_next_token(p), NODE_VIRTUAL_FREE(condition, p->allocator.free_alloc));
        if (parser_current_token_is(p, RPAREN)) {
            IGNORE_STATUS(parser_put_status_error(p,
                                                  IMPROPER_WHILE_CONTINUATION,
                                                  continuation_start.line,
                                                  continuation_start.column));

            NODE_VIRTUAL_FREE(condition, p->allocator.free_alloc);
            return IMPROPER_WHILE_CONTINUATION;
        }
        TRY_DO(expression_parse(p, LOWEST, &continuation),
               NODE_VIRTUAL_FREE(condition, p->allocator.free_alloc));

        TRY_DO(parser_expect_peek(p, RPAREN), {
            NODE_VIRTUAL_FREE(condition, p->allocator.free_alloc);
            NODE_VIRTUAL_FREE(continuation, p->allocator.free_alloc);
        });
    }

    // Now we can parse the block statement now since it has to be here
    TRY_DO(parser_expect_peek(p, LBRACE), {
        NODE_VIRTUAL_FREE(condition, p->allocator.free_alloc);
        NODE_VIRTUAL_FREE(continuation, p->allocator.free_alloc);
    });

    BlockStatement* block;
    TRY_DO(block_statement_parse(p, &block), {
        NODE_VIRTUAL_FREE(condition, p->allocator.free_alloc);
        NODE_VIRTUAL_FREE(continuation, p->allocator.free_alloc);
    });

    // We need at least a continuation or block
    if (!continuation && block->statements.length == 0) {
        IGNORE_STATUS(
            parser_put_status_error(p, EMPTY_WHILE_LOOP, start_token.line, start_token.column));

        NODE_VIRTUAL_FREE(condition, p->allocator.free_alloc);
        NODE_VIRTUAL_FREE(continuation, p->allocator.free_alloc);
        block_statement_destroy((Node*)block, p->allocator.free_alloc);
        return EMPTY_WHILE_LOOP;
    }

    // Finally, we should try to parse the non break clause
    Statement* non_break = NULL;
    if (parser_peek_token_is(p, ELSE)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
        TRY_DO(parser_next_token(p), {
            NODE_VIRTUAL_FREE(condition, p->allocator.free_alloc);
            NODE_VIRTUAL_FREE(continuation, p->allocator.free_alloc);
            block_statement_destroy((Node*)block, p->allocator.free_alloc);
        });

        // This logic follows the same restrictions as if branches and match arms
        switch (p->current_token.type) {
        case VAR:
        case CONST:
        case TYPE:
        case IMPL:
        case IMPORT:
            IGNORE_STATUS(parser_put_status_error(
                p, ILLEGAL_LOOP_NON_BREAK, p->current_token.line, p->current_token.column));

            NODE_VIRTUAL_FREE(condition, p->allocator.free_alloc);
            NODE_VIRTUAL_FREE(continuation, p->allocator.free_alloc);
            block_statement_destroy((Node*)block, p->allocator.free_alloc);
            return ILLEGAL_LOOP_NON_BREAK;
        default:
            TRY_DO(parser_parse_statement(p, &non_break), {
                NODE_VIRTUAL_FREE(condition, p->allocator.free_alloc);
                NODE_VIRTUAL_FREE(continuation, p->allocator.free_alloc);
                block_statement_destroy((Node*)block, p->allocator.free_alloc);
            });
            break;
        }
    }

    // Ownership transfer time
    WhileLoopExpression* while_loop;
    TRY_DO(while_loop_expression_create(start_token,
                                        condition,
                                        continuation,
                                        block,
                                        non_break,
                                        &while_loop,
                                        p->allocator.memory_alloc),
           {
               NODE_VIRTUAL_FREE(condition, p->allocator.free_alloc);
               NODE_VIRTUAL_FREE(continuation, p->allocator.free_alloc);
               block_statement_destroy((Node*)block, p->allocator.free_alloc);
           });

    if (parser_peek_token_is(p, SEMICOLON)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
    }

    *expression = (Expression*)while_loop;
    return SUCCESS;
}

NODISCARD Status do_while_loop_expression_parse(Parser* p, Expression** expression) {
    const Token start_token = p->current_token;
    TRY(parser_expect_peek(p, LBRACE));

    // Parse the block statement which must do some work
    BlockStatement* block;
    TRY(block_statement_parse(p, &block));

    if (block->statements.length == 0) {
        IGNORE_STATUS(
            parser_put_status_error(p, EMPTY_WHILE_LOOP, start_token.line, start_token.column));
        block_statement_destroy((Node*)block, p->allocator.free_alloc);
        return EMPTY_WHILE_LOOP;
    }

    // We have to consume the while token, the open paren, and enter the expression
    TRY_DO(parser_expect_peek(p, WHILE),
           block_statement_destroy((Node*)block, p->allocator.free_alloc));
    TRY_DO(parser_expect_peek(p, LPAREN),
           block_statement_destroy((Node*)block, p->allocator.free_alloc));
    TRY_DO(parser_next_token(p), block_statement_destroy((Node*)block, p->allocator.free_alloc));

    // There's no continuation or non break clause so this is easy :)
    Expression* condition;
    if (parser_current_token_is(p, RPAREN)) {
        IGNORE_STATUS(parser_put_status_error(
            p, WHILE_MISSING_CONDITION, p->current_token.line, p->current_token.column));

        block_statement_destroy((Node*)block, p->allocator.free_alloc);
        return WHILE_MISSING_CONDITION;
    }
    TRY_DO(expression_parse(p, LOWEST, &condition),
           block_statement_destroy((Node*)block, p->allocator.free_alloc));

    // Ownership transfer time
    DoWhileLoopExpression* do_while_loop;
    TRY_DO(do_while_loop_expression_create(
               start_token, block, condition, &do_while_loop, p->allocator.memory_alloc),
           {
               block_statement_destroy((Node*)block, p->allocator.free_alloc);
               NODE_VIRTUAL_FREE(condition, p->allocator.free_alloc);
           });

    TRY_DO(parser_expect_peek(p, RPAREN),
           do_while_loop_expression_destroy((Node*)do_while_loop, p->allocator.free_alloc));

    *expression = (Expression*)do_while_loop;
    return SUCCESS;
}

NODISCARD Status narrow_expression_parse(Parser* p, Expression* outer, Expression** expression) {
    ASSERT_EXPRESSION(outer);
    TRY(parser_expect_peek(p, IDENT));

    Expression* ident_expr;
    TRY(identifier_expression_parse(p, &ident_expr));
    IdentifierExpression* ident = (IdentifierExpression*)ident_expr;

    Node*             outer_node = (Node*)outer;
    NarrowExpression* narrow;
    TRY_DO(narrow_expression_create(
               outer_node->start_token, outer, ident, &narrow, p->allocator.memory_alloc),
           NODE_VIRTUAL_FREE(ident, p->allocator.free_alloc));

    *expression = (Expression*)narrow;
    return SUCCESS;
}

NODISCARD Status assignment_expression_parse(Parser*      p,
                                             Expression*  assignee,
                                             Expression** expression) {
    const TokenType  op_token_type      = p->current_token.type;
    const Precedence current_precedence = parser_current_precedence(p);
    TRY(parser_next_token(p));
    ASSERT_EXPRESSION(assignee);

    Expression* right;
    TRY(expression_parse(p, current_precedence, &right));

    Node*                 assignee_node = (Node*)assignee;
    AssignmentExpression* assign;
    TRY_DO(assignment_expression_create(assignee_node->start_token,
                                        assignee,
                                        op_token_type,
                                        right,
                                        &assign,
                                        p->allocator.memory_alloc),
           NODE_VIRTUAL_FREE(right, p->allocator.free_alloc));

    *expression = (Expression*)assign;
    return SUCCESS;
}
