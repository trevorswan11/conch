#include <assert.h>

#include "lexer/lexer.h"

#include "parser/expression_parsers.h"
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
#include "ast/expressions/index.h"
#include "ast/expressions/infix.h"
#include "ast/expressions/integer.h"
#include "ast/expressions/loop.h"
#include "ast/expressions/match.h"
#include "ast/expressions/namespace.h"
#include "ast/expressions/prefix.h"
#include "ast/expressions/single.h"
#include "ast/expressions/string.h"
#include "ast/expressions/struct.h"
#include "ast/expressions/type.h"
#include "ast/statements/block.h"
#include "ast/statements/statement.h"

#include "util/containers/string_builder.h"

#define TRAILING_ALTERNATE(result, cleanup, err_code)                          \
    switch (p->current_token.type) {                                           \
    case VAR:                                                                  \
    case CONST:                                                                \
    case TYPE:                                                                 \
    case IMPL:                                                                 \
    case IMPORT:                                                               \
    case UNDERSCORE:                                                           \
        PUT_STATUS_PROPAGATE(&p->errors, err_code, p->current_token, cleanup); \
    default:                                                                   \
        TRY_DO(parser_parse_statement(p, &result), cleanup);                   \
        break;                                                                 \
    }

#define POSSIBLE_TRAILING_ALTERNATE(result, cleanup, err_code)                  \
    if (parser_peek_token_is(p, ELSE)) {                                        \
        UNREACHABLE_IF_ERROR(parser_next_token(p));                             \
        switch (p->peek_token.type) {                                           \
        case VAR:                                                               \
        case CONST:                                                             \
        case TYPE:                                                              \
        case IMPL:                                                              \
        case IMPORT:                                                            \
        case UNDERSCORE:                                                        \
            PUT_STATUS_PROPAGATE(&p->errors, err_code, p->peek_token, cleanup); \
        default:                                                                \
            TRY_DO(parser_next_token(p), cleanup);                              \
            TRY_DO(parser_parse_statement(p, &result), cleanup);                \
            break;                                                              \
        }                                                                       \
    }

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
        if (!poll_infix(p, p->peek_token.type, &infix)) { return SUCCESS; }

        TRY(parser_next_token(p));
        if (p->peek_token.type == END) {
            PUT_STATUS_PROPAGATE(&p->errors,
                                 INFIX_MISSING_RHS,
                                 p->current_token,
                                 NODE_VIRTUAL_FREE(*lhs_expression, p->allocator.free_alloc));
        }

        TRY_DO(infix.infix_parse(p, *lhs_expression, lhs_expression),
               NODE_VIRTUAL_FREE(*lhs_expression, p->allocator.free_alloc));
    }

    return SUCCESS;
}

NODISCARD Status identifier_expression_parse(Parser* p, Expression** expression) {
    const Token start_token = p->current_token;
    if (start_token.type != IDENT && !hash_set_contains(&p->primitives, &start_token.type)) {
        PUT_STATUS_PROPAGATE(&p->errors, ILLEGAL_IDENTIFIER, start_token, {});
    }

    MutSlice name;
    TRY(slice_dupe(&name, &start_token.slice, p->allocator.memory_alloc));

    IdentifierExpression* ident;
    TRY_DO(identifier_expression_create(start_token, name, &ident, p->allocator.memory_alloc),
           p->allocator.free_alloc(name.ptr));
    *expression = (Expression*)ident;
    return SUCCESS;
}

NODISCARD Status parameter_list_parse(Parser*    p,
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
            Expression* ident_expr;
            TRY_DO(identifier_expression_parse(p, &ident_expr),
                   free_parameter_list(&list, p->allocator.free_alloc));
            IdentifierExpression* ident = (IdentifierExpression*)ident_expr;

            Expression* type_expr;
            bool        initalized;
            TRY_DO(type_expression_parse(p, &type_expr, &initalized), {
                NODE_VIRTUAL_FREE(ident, allocator.free_alloc);
                free_parameter_list(&list, p->allocator.free_alloc);
            });

            TypeExpression* type = (TypeExpression*)type_expr;
            if (type->tag == IMPLICIT) {
                PUT_STATUS_PROPAGATE(&p->errors, IMPLICIT_FN_PARAM_TYPE, p->current_token, {
                    NODE_VIRTUAL_FREE(ident, allocator.free_alloc);
                    NODE_VIRTUAL_FREE(type, allocator.free_alloc);
                    free_parameter_list(&list, p->allocator.free_alloc);
                });
            }

            // Parse the default value based on the types knowledge of it
            Expression* default_value = NULL;
            if (initalized) {
                some_initialized = some_initialized || initalized;

                TRY_DO(expression_parse(p, LOWEST, &default_value), {
                    NODE_VIRTUAL_FREE(ident, allocator.free_alloc);
                    NODE_VIRTUAL_FREE(type, allocator.free_alloc);
                    free_parameter_list(&list, p->allocator.free_alloc);
                });

                // Expression parsing moves up to the end of the expression, so pass it
                TRY_DO(parser_next_token(p), {
                    NODE_VIRTUAL_FREE(ident, allocator.free_alloc);
                    NODE_VIRTUAL_FREE(type, allocator.free_alloc);
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
                NODE_VIRTUAL_FREE(ident, allocator.free_alloc);
                NODE_VIRTUAL_FREE(type, allocator.free_alloc);
                NODE_VIRTUAL_FREE(default_value, p->allocator.free_alloc);
                free_parameter_list(&list, p->allocator.free_alloc);
            });

            // Parsing a type may move up to the closing parentheses
            if (parser_current_token_is(p, RPAREN)) { break; }

            // Consume the comma and advance past it if there's not a closing delim
            TRY_DO(parser_expect_current(p, COMMA),
                   free_parameter_list(&list, p->allocator.free_alloc));
        }
    }

    // The caller may not care about default parameter presence
    if (contains_default_param) { *contains_default_param = some_initialized; }

    *parameters = list;
    return SUCCESS;
}

NODISCARD Status generics_parse(Parser* p, ArrayList* generics) {
    TRY(array_list_init_allocator(generics, 1, sizeof(Expression*), p->allocator));
    if (parser_peek_token_is(p, LT)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
        if (parser_peek_token_is(p, GT)) {
            PUT_STATUS_PROPAGATE(&p->errors,
                                 EMPTY_GENERIC_LIST,
                                 p->current_token,
                                 free_expression_list(generics, p->allocator.free_alloc));
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
    TRY_DO(parameter_list_parse(p, parameters, contains_default_param),
           free_expression_list(generics, p->allocator.free_alloc));

    TRY_DO(parser_expect_peek(p, COLON), {
        free_parameter_list(parameters, p->allocator.free_alloc);
        free_expression_list(generics, p->allocator.free_alloc);
    });
    const Token type_token_start = p->current_token;

    if (STATUS_ERR(explicit_type_parse(p, type_token_start, return_type))) {
        PUT_STATUS_PROPAGATE(&p->errors, MALFORMED_FUNCTION_LITERAL, type_token_start, {
            free_parameter_list(parameters, p->allocator.free_alloc);
            free_expression_list(generics, p->allocator.free_alloc);
        });
    }

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

        TRY(array_list_init_allocator(&dim_array, 1, sizeof(size_t), p->allocator));

        while (!parser_peek_token_is(p, RBRACKET) && !parser_peek_token_is(p, END)) {
            UNREACHABLE_IF_ERROR(parser_next_token(p));
            const Token integer_token = p->current_token;

            if (!token_is_size_integer(integer_token.type)) {
                PUT_STATUS_PROPAGATE(&p->errors,
                                     UNEXPECTED_ARRAY_SIZE_TOKEN,
                                     integer_token,
                                     array_list_deinit(&dim_array));
            }

            size_t dim;
            TRY_DO(strntouz(integer_token.slice.ptr,
                            integer_token.slice.length - 2,
                            integer_token_to_base(integer_token.type),
                            &dim),
                   array_list_deinit(&dim_array));

            TRY_DO(array_list_push(&dim_array, &dim), array_list_deinit(&dim_array));

            if (parser_peek_token_is(p, COMMA)) { UNREACHABLE_IF_ERROR(parser_next_token(p)); }
        }

        TRY_DO(parser_expect_peek(p, RBRACKET), array_list_deinit(&dim_array));
        if (dim_array.length == 0) {
            PUT_STATUS_PROPAGATE(
                &p->errors, MISSING_ARRAY_SIZE_TOKEN, start_token, array_list_deinit(&dim_array));
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
    const bool          is_primitive   = hash_set_contains(&p->primitives, &p->peek_token.type);
    TypeExpressionUnion explicit_union = (TypeExpressionUnion){
        .explicit_type =
            (ExplicitType){
                .nullable  = is_inner_type_nullable,
                .primitive = is_primitive,
            },
    };

    if (is_primitive || parser_peek_token_is(p, IDENT)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));

        Expression* ident_expr;
        TRY_DO(identifier_expression_parse(p, &ident_expr), array_list_deinit(&dim_array));

        ArrayList generics;
        TRY_DO(generics_parse(p, &generics), {
            NODE_VIRTUAL_FREE(ident_expr, p->allocator.free_alloc);
            array_list_deinit(&dim_array);
        });

        explicit_union.explicit_type.tag     = EXPLICIT_IDENT;
        explicit_union.explicit_type.variant = (ExplicitTypeUnion){
            (ExplicitIdentType){
                .name     = (IdentifierExpression*)ident_expr,
                .generics = generics,
            },
        };

        TRY_DO(type_expression_create(
                   start_token, EXPLICIT, explicit_union, type, p->allocator.memory_alloc),
               {
                   NODE_VIRTUAL_FREE(ident_expr, p->allocator.free_alloc);
                   array_list_deinit(&dim_array);
                   free_expression_list(&generics, p->allocator.free_alloc);
               });
    } else {
        const Token type_start = p->current_token;
        UNREACHABLE_IF_ERROR(parser_next_token(p));

        switch (p->current_token.type) {
        case TYPEOF: {
            // Using arrays, functions, enums, or structs now is weird
            switch (p->peek_token.type) {
            case LBRACKET:
            case FUNCTION:
            case ENUM:
            case STRUCT:
                PUT_STATUS_PROPAGATE(&p->errors,
                                     REDUNDANT_TYPE_INTROSPECTION,
                                     p->peek_token,
                                     array_list_deinit(&dim_array));
            default:
                TRY_DO(parser_next_token(p), array_list_deinit(&dim_array));
                break;
            }

            Expression* referential_type;
            TRY_DO(expression_parse(p, LOWEST, &referential_type), array_list_deinit(&dim_array));

            explicit_union.explicit_type.tag     = EXPLICIT_TYPEOF;
            explicit_union.explicit_type.variant = (ExplicitTypeUnion){
                .referred_type = referential_type,
            };

            TRY_DO(type_expression_create(
                       start_token, EXPLICIT, explicit_union, type, p->allocator.memory_alloc),
                   {
                       NODE_VIRTUAL_FREE(referential_type, p->allocator.free_alloc);
                       array_list_deinit(&dim_array);
                   });
            break;
        }
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
                TRY_DO(
                    put_status_error(
                        &p->errors, MALFORMED_FUNCTION_LITERAL, type_start.line, type_start.column),
                    {
                        free_expression_list(&generics, p->allocator.free_alloc);
                        free_parameter_list(&parameters, p->allocator.free_alloc);
                        NODE_VIRTUAL_FREE(return_type, p->allocator.free_alloc);
                        array_list_deinit(&dim_array);
                    });
            }

            explicit_union.explicit_type.tag     = EXPLICIT_FN;
            explicit_union.explicit_type.variant = (ExplicitTypeUnion){
                .function_type =
                    (ExplicitFunctionType){
                        .generics    = generics,
                        .parameters  = parameters,
                        .return_type = return_type,
                    },
            };

            TRY_DO(type_expression_create(
                       start_token, EXPLICIT, explicit_union, type, p->allocator.memory_alloc),
                   {
                       free_expression_list(&generics, p->allocator.free_alloc);
                       free_parameter_list(&parameters, p->allocator.free_alloc);
                       NODE_VIRTUAL_FREE(return_type, p->allocator.free_alloc);
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
                       NODE_VIRTUAL_FREE(struct_type, p->allocator.free_alloc);
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
                       NODE_VIRTUAL_FREE(enum_type, p->allocator.free_alloc);
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
        TypeExpressionUnion array_type_union = (TypeExpressionUnion){
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
                   NODE_VIRTUAL_FREE(*type, p->allocator.free_alloc);
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

#define INT_PARSE(T, t, N, parse_fn, create_fn)                                   \
    t            value;                                                           \
    const Status parse_status = parse_fn(start_token.slice.ptr, N, base, &value); \
    if (STATUS_ERR(parse_status)) {                                               \
        PUT_STATUS_PROPAGATE(&p->errors, parse_status, start_token, {});          \
    }                                                                             \
                                                                                  \
    T* integer;                                                                   \
    TRY(create_fn(start_token, value, &integer, p->allocator.memory_alloc));      \
    *expression = (Expression*)integer;

NODISCARD Status integer_literal_expression_parse(Parser* p, Expression** expression) {
    const Token start_token = p->current_token;
    const int   base        = integer_token_to_base(start_token.type);

    if (token_is_signed_integer(start_token.type)) {
        assert(start_token.slice.length > 0);
        INT_PARSE(IntegerLiteralExpression,
                  int64_t,
                  start_token.slice.length,
                  strntoll,
                  integer_literal_expression_create);
    } else if (token_is_unsigned_integer(start_token.type)) {
        assert(start_token.slice.length > 1);
        INT_PARSE(UnsignedIntegerLiteralExpression,
                  uint64_t,
                  start_token.slice.length - 1,
                  strntoull,
                  uinteger_literal_expression_create);
    } else if (token_is_size_integer(start_token.type)) {
        assert(start_token.slice.length > 2);
        INT_PARSE(SizeIntegerLiteralExpression,
                  size_t,
                  start_token.slice.length - 2,
                  strntouz,
                  uzinteger_literal_expression_create);
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
        PUT_STATUS_PROPAGATE(&p->errors, parse_status, start_token, {});
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

    if (p->peek_token.type == END) {
        PUT_STATUS_PROPAGATE(&p->errors, PREFIX_MISSING_OPERAND, p->current_token, {});
    }
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

static inline NODISCARD Status if_expression_parse_branch(Parser* p, Statement** stmt) {
    TRY(parser_next_token(p));

    Statement* alternate;
    TRAILING_ALTERNATE(alternate, {}, ILLEGAL_IF_BRANCH);
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
    TRY_DO(if_expression_parse_branch(p, &consequence),
           NODE_VIRTUAL_FREE(condition, p->allocator.free_alloc));

    Statement* alternate = NULL;
    if (parser_peek_token_is(p, ELSE)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
        TRY_DO(if_expression_parse_branch(p, &alternate), {
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
        NODE_VIRTUAL_FREE(return_type, p->allocator.free_alloc);
    });

    BlockStatement* body;
    TRY_DO(block_statement_parse(p, &body), {
        free_expression_list(&generics, p->allocator.free_alloc);
        free_parameter_list(&parameters, p->allocator.free_alloc);
        NODE_VIRTUAL_FREE(return_type, p->allocator.free_alloc);
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
               NODE_VIRTUAL_FREE(return_type, p->allocator.free_alloc);
               free_parameter_list(&parameters, p->allocator.free_alloc);
               NODE_VIRTUAL_FREE(body, p->allocator.free_alloc);
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

NODISCARD Status index_expression_parse(Parser* p, Expression* array, Expression** expression) {
    const Token start_token = p->current_token;
    ASSERT_EXPRESSION(array);

    if (parser_peek_token_is(p, RBRACE)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
        PUT_STATUS_PROPAGATE(&p->errors, INDEX_MISSING_EXPRESSION, start_token, {});
    }

    TRY(parser_next_token(p));
    Expression* idx_expr;
    TRY(expression_parse(p, LOWEST, &idx_expr));

    IndexExpression* index;
    TRY_DO(index_expression_create(start_token, array, idx_expr, &index, p->allocator.memory_alloc),
           NODE_VIRTUAL_FREE(idx_expr, p->allocator.free_alloc));

    TRY_DO(parser_expect_peek(p, RBRACKET), NODE_VIRTUAL_FREE(index, p->allocator.free_alloc));

    *expression = (Expression*)index;
    return SUCCESS;
}

NODISCARD Status struct_expression_parse(Parser* p, Expression** expression) {
    const Token start_token = p->current_token;
    ArrayList   generics;
    TRY(generics_parse(p, &generics));

    TRY_DO(parser_expect_peek(p, LBRACE), free_expression_list(&generics, p->allocator.free_alloc));
    if (parser_peek_token_is(p, RBRACE)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
        PUT_STATUS_PROPAGATE(&p->errors,
                             STRUCT_MISSING_MEMBERS,
                             start_token,
                             free_expression_list(&generics, p->allocator.free_alloc));
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
            NODE_VIRTUAL_FREE(ident_expr, p->allocator.free_alloc);
            free_struct_member_list(&members, p->allocator.free_alloc);
            free_expression_list(&generics, p->allocator.free_alloc);
        });

        // Struct members must have explicit type declarations
        TypeExpression* type = (TypeExpression*)type_expr;
        if (type->tag == IMPLICIT) {
            PUT_STATUS_PROPAGATE(&p->errors, STRUCT_MEMBER_NOT_EXPLICIT, p->current_token, {
                NODE_VIRTUAL_FREE(ident_expr, p->allocator.free_alloc);
                NODE_VIRTUAL_FREE(type_expr, p->allocator.free_alloc);
                free_struct_member_list(&members, p->allocator.free_alloc);
                free_expression_list(&generics, p->allocator.free_alloc);
            });
        }

        // The type parser leaves with the current token on assignment in this case
        Expression* default_value = NULL;
        if (has_default_value) {
            TRY_DO(expression_parse(p, LOWEST, &default_value), {
                NODE_VIRTUAL_FREE(ident_expr, p->allocator.free_alloc);
                NODE_VIRTUAL_FREE(type_expr, p->allocator.free_alloc);
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
            NODE_VIRTUAL_FREE(ident_expr, p->allocator.free_alloc);
            NODE_VIRTUAL_FREE(type_expr, p->allocator.free_alloc);
            NODE_VIRTUAL_FREE(default_value, p->allocator.free_alloc);
            free_struct_member_list(&members, p->allocator.free_alloc);
            free_expression_list(&generics, p->allocator.free_alloc);
        });

        // All members require a trailing comma!
        if (has_default_value && parser_peek_token_is(p, COMMA)) {
            UNREACHABLE_IF_ERROR(parser_next_token(p));
        } else if (!parser_current_token_is(p, COMMA)) {
            PUT_STATUS_PROPAGATE(&p->errors, MISSING_TRAILING_COMMA, p->current_token, {
                free_struct_member_list(&members, p->allocator.free_alloc);
                free_expression_list(&generics, p->allocator.free_alloc);
            });
        }
    }

    TRY_DO(parser_expect_peek(p, RBRACE), {
        free_struct_member_list(&members, p->allocator.free_alloc);
        free_expression_list(&generics, p->allocator.free_alloc);
    });

    if (parser_peek_token_is(p, SEMICOLON)) { UNREACHABLE_IF_ERROR(parser_next_token(p)); }

    StructExpression* struct_expr;
    TRY_DO(struct_expression_create(
               start_token, generics, members, &struct_expr, p->allocator.memory_alloc),
           {
               free_struct_member_list(&members, p->allocator.free_alloc);
               free_expression_list(&generics, p->allocator.free_alloc);
           });

    *expression = (Expression*)struct_expr;
    return SUCCESS;
}

NODISCARD Status enum_expression_parse(Parser* p, Expression** expression) {
    const Token start_token = p->current_token;
    TRY(parser_expect_peek(p, LBRACE));
    if (parser_peek_token_is(p, RBRACE)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
        PUT_STATUS_PROPAGATE(&p->errors, ENUM_MISSING_VARIANTS, start_token, {});
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
                NODE_VIRTUAL_FREE(ident, p->allocator.free_alloc);
            });

            TRY_DO(expression_parse(p, LOWEST, &value), {
                free_enum_variant_list(&variants, p->allocator.free_alloc);
                NODE_VIRTUAL_FREE(ident, p->allocator.free_alloc);
            });
        }

        EnumVariant variant = (EnumVariant){.name = ident, .value = value};
        TRY_DO(array_list_push(&variants, &variant), {
            free_enum_variant_list(&variants, p->allocator.free_alloc);
            NODE_VIRTUAL_FREE(ident, p->allocator.free_alloc);
            NODE_VIRTUAL_FREE(value, p->allocator.free_alloc);
        });

        // All variants require a trailing comma!
        if (STATUS_ERR(parser_expect_peek(p, COMMA))) {
            PUT_STATUS_PROPAGATE(&p->errors,
                                 MISSING_TRAILING_COMMA,
                                 p->current_token,
                                 free_enum_variant_list(&variants, p->allocator.free_alloc));
        }
    }

    TRY_DO(parser_expect_peek(p, RBRACE),
           free_enum_variant_list(&variants, p->allocator.free_alloc));
    if (parser_peek_token_is(p, SEMICOLON)) { UNREACHABLE_IF_ERROR(parser_next_token(p)); }

    EnumExpression* enum_expr;
    TRY_DO(enum_expression_create(start_token, variants, &enum_expr, p->allocator.memory_alloc),
           free_enum_variant_list(&variants, p->allocator.free_alloc));

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

    if (parser_peek_token_is(p, RBRACE)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
        PUT_STATUS_PROPAGATE(&p->errors,
                             ARMLESS_MATCH_EXPR,
                             start_token,
                             NODE_VIRTUAL_FREE(match_cond_expr, p->allocator.free_alloc));
    }

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
        TRAILING_ALTERNATE(
            consequence,
            {
                NODE_VIRTUAL_FREE(match_cond_expr, p->allocator.free_alloc);
                NODE_VIRTUAL_FREE(pattern, p->allocator.free_alloc);
                free_match_arm_list(&arms, p->allocator.free_alloc);
            },
            ILLEGAL_MATCH_ARM);

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
        PUT_STATUS_PROPAGATE(&p->errors, ARMLESS_MATCH_EXPR, start_token, {
            NODE_VIRTUAL_FREE(match_cond_expr, p->allocator.free_alloc);
            free_match_arm_list(&arms, p->allocator.free_alloc);
        });
    }

    TRY_DO(parser_expect_peek(p, RBRACE), {
        NODE_VIRTUAL_FREE(match_cond_expr, p->allocator.free_alloc);
        free_match_arm_list(&arms, p->allocator.free_alloc);
    });

    // Catch all statement is optional and is the equivalent of a switch default
    Statement* catch_all = NULL;
    POSSIBLE_TRAILING_ALTERNATE(
        catch_all,
        {
            NODE_VIRTUAL_FREE(match_cond_expr, p->allocator.free_alloc);
            free_match_arm_list(&arms, p->allocator.free_alloc);
        },
        ILLEGAL_MATCH_CATCH_ALL);

    MatchExpression* match;
    TRY_DO(match_expression_create(
               start_token, match_cond_expr, arms, catch_all, &match, p->allocator.memory_alloc),
           {
               NODE_VIRTUAL_FREE(match_cond_expr, p->allocator.free_alloc);
               free_match_arm_list(&arms, p->allocator.free_alloc);
           });

    if (parser_peek_token_is(p, SEMICOLON)) { UNREACHABLE_IF_ERROR(parser_next_token(p)); }

    *expression = (Expression*)match;
    return SUCCESS;
}

NODISCARD Status array_literal_expression_parse(Parser* p, Expression** expression) {
    const Token start_token = p->current_token;
    TRY(parser_next_token(p));

    size_t array_size;
    if (p->current_token.type == UNDERSCORE) {
        array_size = 0;
    } else {
        const Token integer_token = p->current_token;

        // Before parsing, check if we even have a value here
        if (parser_current_token_is(p, RBRACKET)) {
            PUT_STATUS_PROPAGATE(&p->errors, MISSING_ARRAY_SIZE_TOKEN, integer_token, {});
        }

        if (!token_is_size_integer(integer_token.type)) {
            PUT_STATUS_PROPAGATE(&p->errors, UNEXPECTED_ARRAY_SIZE_TOKEN, integer_token, {});
        }

        TRY(strntouz(integer_token.slice.ptr,
                     integer_token.slice.length - 2,
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
        PUT_STATUS_PROPAGATE(&p->errors,
                             INCORRECT_EXPLICIT_ARRAY_SIZE,
                             start_token,
                             free_expression_list(&items, p->allocator.free_alloc));
    }

    ArrayLiteralExpression* array;
    TRY_DO(array_literal_expression_create(
               start_token, inferred_size, items, &array, p->allocator.memory_alloc),
           free_expression_list(&items, p->allocator.free_alloc));

    TRY_DO(parser_expect_peek(p, RBRACE), NODE_VIRTUAL_FREE(array, p->allocator.free_alloc));
    if (parser_peek_token_is(p, SEMICOLON)) { UNREACHABLE_IF_ERROR(parser_next_token(p)); }

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
        PUT_STATUS_PROPAGATE(&p->errors,
                             FOR_MISSING_ITERABLES,
                             start_token,
                             free_expression_list(&iterables, p->allocator.free_alloc));
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
        PUT_STATUS_PROPAGATE(&p->errors, FOR_ITERABLE_CAPTURE_MISMATCH, start_token, {
            free_expression_list(&iterables, p->allocator.free_alloc);
            free_for_capture_list(&captures, p->allocator.free_alloc);
        });
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
        PUT_STATUS_PROPAGATE(&p->errors, EMPTY_FOR_LOOP, start_token, {
            free_expression_list(&iterables, p->allocator.free_alloc);
            free_for_capture_list(&captures, p->allocator.free_alloc);
            NODE_VIRTUAL_FREE(block, p->allocator.free_alloc);
        });
    }

    // Finally, we should try to parse the non break clause
    Statement* non_break = NULL;
    POSSIBLE_TRAILING_ALTERNATE(
        non_break,
        {
            free_expression_list(&iterables, p->allocator.free_alloc);
            free_for_capture_list(&captures, p->allocator.free_alloc);
            NODE_VIRTUAL_FREE(block, p->allocator.free_alloc);
        },
        ILLEGAL_LOOP_NON_BREAK);

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
               NODE_VIRTUAL_FREE(block, p->allocator.free_alloc);
               NODE_VIRTUAL_FREE(non_break, p->allocator.free_alloc);
           });

    if (parser_peek_token_is(p, SEMICOLON)) { UNREACHABLE_IF_ERROR(parser_next_token(p)); }

    *expression = (Expression*)for_loop;
    return SUCCESS;
}

NODISCARD Status while_loop_expression_parse(Parser* p, Expression** expression) {
    const Token start_token = p->current_token;
    TRY(parser_expect_peek(p, LPAREN));
    TRY(parser_next_token(p));

    Expression* condition;
    if (parser_current_token_is(p, RPAREN)) {
        PUT_STATUS_PROPAGATE(&p->errors, WHILE_MISSING_CONDITION, p->current_token, {});
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
            PUT_STATUS_PROPAGATE(&p->errors,
                                 IMPROPER_WHILE_CONTINUATION,
                                 continuation_start,
                                 NODE_VIRTUAL_FREE(condition, p->allocator.free_alloc));
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
        PUT_STATUS_PROPAGATE(&p->errors, EMPTY_WHILE_LOOP, start_token, {
            NODE_VIRTUAL_FREE(condition, p->allocator.free_alloc);
            NODE_VIRTUAL_FREE(continuation, p->allocator.free_alloc);
            NODE_VIRTUAL_FREE(block, p->allocator.free_alloc);
        });
    }

    // Finally, we should try to parse the non break clause
    Statement* non_break = NULL;
    POSSIBLE_TRAILING_ALTERNATE(
        non_break,
        {
            NODE_VIRTUAL_FREE(condition, p->allocator.free_alloc);
            NODE_VIRTUAL_FREE(continuation, p->allocator.free_alloc);
            NODE_VIRTUAL_FREE(block, p->allocator.free_alloc);
        },
        ILLEGAL_LOOP_NON_BREAK);

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
               NODE_VIRTUAL_FREE(block, p->allocator.free_alloc);
           });

    if (parser_peek_token_is(p, SEMICOLON)) { UNREACHABLE_IF_ERROR(parser_next_token(p)); }

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
        PUT_STATUS_PROPAGATE(&p->errors,
                             EMPTY_WHILE_LOOP,
                             start_token,
                             NODE_VIRTUAL_FREE(block, p->allocator.free_alloc));
    }

    // We have to consume the while token, the open paren, and enter the expression
    TRY_DO(parser_expect_peek(p, WHILE), NODE_VIRTUAL_FREE(block, p->allocator.free_alloc));
    TRY_DO(parser_expect_peek(p, LPAREN), NODE_VIRTUAL_FREE(block, p->allocator.free_alloc));
    TRY_DO(parser_next_token(p), NODE_VIRTUAL_FREE(block, p->allocator.free_alloc));

    // There's no continuation or non break clause so this is easy :)
    Expression* condition;
    if (parser_current_token_is(p, RPAREN)) {
        PUT_STATUS_PROPAGATE(&p->errors,
                             WHILE_MISSING_CONDITION,
                             p->current_token,
                             NODE_VIRTUAL_FREE(block, p->allocator.free_alloc));
    }
    TRY_DO(expression_parse(p, LOWEST, &condition),
           NODE_VIRTUAL_FREE(block, p->allocator.free_alloc));

    // Ownership transfer time
    DoWhileLoopExpression* do_while_loop;
    TRY_DO(do_while_loop_expression_create(
               start_token, block, condition, &do_while_loop, p->allocator.memory_alloc),
           {
               NODE_VIRTUAL_FREE(block, p->allocator.free_alloc);
               NODE_VIRTUAL_FREE(condition, p->allocator.free_alloc);
           });

    TRY_DO(parser_expect_peek(p, RPAREN),
           NODE_VIRTUAL_FREE(do_while_loop, p->allocator.free_alloc));

    *expression = (Expression*)do_while_loop;
    return SUCCESS;
}

NODISCARD Status namespace_expression_parse(Parser* p, Expression* outer, Expression** expression) {
    ASSERT_EXPRESSION(outer);
    TRY(parser_expect_peek(p, IDENT));

    Expression* ident_expr;
    TRY(identifier_expression_parse(p, &ident_expr));
    IdentifierExpression* ident = (IdentifierExpression*)ident_expr;

    Node*                outer_node = (Node*)outer;
    NamespaceExpression* namespace_expr;
    TRY_DO(namespace_expression_create(
               outer_node->start_token, outer, ident, &namespace_expr, p->allocator.memory_alloc),
           NODE_VIRTUAL_FREE(ident, p->allocator.free_alloc));

    *expression = (Expression*)namespace_expr;
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
