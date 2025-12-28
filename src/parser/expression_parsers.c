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
#include "util/status.h"

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
        TRY_DO(parser_parse_statement(p, &(result)), cleanup);                 \
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
            TRY_DO(parser_parse_statement(p, &(result)), cleanup);              \
            break;                                                              \
        }                                                                       \
    }

[[nodiscard]] static inline Status record_missing_prefix(Parser* p) {
    Allocator*    allocator = parser_allocator(p);
    const Token   current   = p->current_token;
    StringBuilder sb;
    TRY(string_builder_init_allocator(&sb, 50, allocator));

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

[[nodiscard]] Status
expression_parse(Parser* p, Precedence precedence, Expression** lhs_expression) {
    Allocator* allocator = parser_allocator(p);

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
                                 NODE_VIRTUAL_FREE(*lhs_expression, allocator));
        }

        TRY_DO(infix.infix_parse(p, *lhs_expression, lhs_expression),
               NODE_VIRTUAL_FREE(*lhs_expression, allocator));
    }

    return SUCCESS;
}

[[nodiscard]] Status identifier_expression_parse(Parser* p, Expression** expression) {
    Allocator*  allocator   = parser_allocator(p);
    const Token start_token = p->current_token;
    if (start_token.type != IDENT && !hash_set_contains(&p->primitives, &start_token.type)) {
        PUT_STATUS_PROPAGATE(&p->errors, ILLEGAL_IDENTIFIER, start_token, {});
    }

    MutSlice name;
    TRY(slice_dupe(&name, &start_token.slice, allocator));

    IdentifierExpression* ident;
    TRY_DO(identifier_expression_create(start_token, name, &ident, allocator),
           ALLOCATOR_PTR_FREE(allocator, name.ptr));
    *expression = (Expression*)ident;
    return SUCCESS;
}

[[nodiscard]] Status
parameter_list_parse(Parser* p, ArrayList* parameters, bool* contains_default_param) {
    Allocator* allocator = parser_allocator(p);

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
                   free_parameter_list(&list, allocator));
            IdentifierExpression* ident = (IdentifierExpression*)ident_expr;

            Expression* type_expr;
            bool        initalized;
            TRY_DO(type_expression_parse(p, &type_expr, &initalized), {
                NODE_VIRTUAL_FREE(ident, allocator);
                free_parameter_list(&list, allocator);
            });

            TypeExpression* type = (TypeExpression*)type_expr;
            if (type->tag == IMPLICIT) {
                PUT_STATUS_PROPAGATE(&p->errors, IMPLICIT_FN_PARAM_TYPE, p->current_token, {
                    NODE_VIRTUAL_FREE(ident, allocator);
                    NODE_VIRTUAL_FREE(type, allocator);
                    free_parameter_list(&list, allocator);
                });
            }

            // Parse the default value based on the types knowledge of it
            Expression* default_value = nullptr;
            if (initalized) {
                some_initialized = some_initialized || initalized;

                TRY_DO(expression_parse(p, LOWEST, &default_value), {
                    NODE_VIRTUAL_FREE(ident, allocator);
                    NODE_VIRTUAL_FREE(type, allocator);
                    free_parameter_list(&list, allocator);
                });

                // Expression parsing moves up to the end of the expression, so pass it
                TRY_DO(parser_next_token(p), {
                    NODE_VIRTUAL_FREE(ident, allocator);
                    NODE_VIRTUAL_FREE(type, allocator);
                    NODE_VIRTUAL_FREE(default_value, allocator);
                    free_parameter_list(&list, allocator);
                });
            }

            Parameter parameter = {
                .is_ref        = is_ref_argument,
                .ident         = ident,
                .type          = type,
                .default_value = default_value,
            };

            TRY_DO(array_list_push(&list, &parameter), {
                NODE_VIRTUAL_FREE(ident, allocator);
                NODE_VIRTUAL_FREE(type, allocator);
                NODE_VIRTUAL_FREE(default_value, allocator);
                free_parameter_list(&list, allocator);
            });

            // Parsing a type may move up to the closing parentheses
            if (parser_current_token_is(p, RPAREN)) { break; }

            // Consume the comma and advance past it if there's not a closing delim
            TRY_DO(parser_expect_current(p, COMMA), free_parameter_list(&list, allocator));
        }
    }

    // The caller may not care about default parameter presence
    if (contains_default_param) { *contains_default_param = some_initialized; }

    *parameters = list;
    return SUCCESS;
}

[[nodiscard]] Status generics_parse(Parser* p, ArrayList* generics) {
    Allocator* allocator = parser_allocator(p);
    TRY(array_list_init_allocator(generics, 1, sizeof(Expression*), allocator));
    if (parser_peek_token_is(p, LT)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
        if (parser_peek_token_is(p, GT)) {
            PUT_STATUS_PROPAGATE(&p->errors,
                                 EMPTY_GENERIC_LIST,
                                 p->current_token,
                                 free_expression_list(generics, allocator));
        }

        while (!parser_peek_token_is(p, GT) && !parser_peek_token_is(p, END)) {
            UNREACHABLE_IF_ERROR(parser_next_token(p));

            Expression* ident;
            TRY_DO(identifier_expression_parse(p, &ident),
                   free_expression_list(generics, allocator));

            TRY_DO(array_list_push(generics, (const void*)&ident), {
                free_expression_list(generics, allocator);
                NODE_VIRTUAL_FREE(ident, allocator);
            });

            if (!parser_peek_token_is(p, GT)) {
                TRY_DO(parser_expect_peek(p, COMMA), free_expression_list(generics, allocator));
            }
        }

        TRY_DO(parser_expect_peek(p, GT), free_expression_list(generics, allocator));
    }

    return SUCCESS;
}

[[nodiscard]] Status function_definition_parse(Parser*          p,
                                               ArrayList*       generics,
                                               ArrayList*       parameters,
                                               TypeExpression** return_type,
                                               bool*            contains_default_param) {
    Allocator* allocator = parser_allocator(p);
    TRY(generics_parse(p, generics));

    TRY_DO(parser_expect_peek(p, LPAREN), free_expression_list(generics, allocator));
    TRY_DO(parameter_list_parse(p, parameters, contains_default_param),
           free_expression_list(generics, allocator));

    TRY_DO(parser_expect_peek(p, COLON), {
        free_parameter_list(parameters, allocator);
        free_expression_list(generics, allocator);
    });
    const Token type_token_start = p->current_token;

    if (STATUS_ERR(explicit_type_parse(p, type_token_start, return_type))) {
        PUT_STATUS_PROPAGATE(&p->errors, MALFORMED_FUNCTION_LITERAL, type_token_start, {
            free_parameter_list(parameters, allocator);
            free_expression_list(generics, allocator);
        });
    }

    return SUCCESS;
}

[[nodiscard]] Status explicit_type_parse(Parser* p, Token start_token, TypeExpression** type) {
    Allocator* allocator = parser_allocator(p);

    // Check for a question mark and to allow nil
    bool is_nullable = false;
    if (parser_peek_token_is(p, WHAT)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
        is_nullable = true;
    }

    // Arrays are a little weird especially with the function signature
    ArrayList dim_array;
    dim_array.data              = nullptr;
    bool is_array_type          = false;
    bool is_inner_type_nullable = false;

    if (parser_peek_token_is(p, LBRACKET)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));

        TRY(array_list_init_allocator(&dim_array, 1, sizeof(size_t), allocator));

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

            if (dim == 0) {
                PUT_STATUS_PROPAGATE(
                    &p->errors, EMPTY_ARRAY, integer_token, array_list_deinit(&dim_array));
            }

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
        // If we don't have an array, the inner type is just the normal type and needs to be set
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
            NODE_VIRTUAL_FREE(ident_expr, allocator);
            array_list_deinit(&dim_array);
        });

        explicit_union.explicit_type.tag     = EXPLICIT_IDENT;
        explicit_union.explicit_type.variant = (ExplicitTypeUnion){
            (ExplicitIdentType){
                .name     = (IdentifierExpression*)ident_expr,
                .generics = generics,
            },
        };

        TRY_DO(type_expression_create(start_token, EXPLICIT, explicit_union, type, allocator), {
            NODE_VIRTUAL_FREE(ident_expr, allocator);
            array_list_deinit(&dim_array);
            free_expression_list(&generics, allocator);
        });
    } else {
        const Token type_start = p->current_token;
        UNREACHABLE_IF_ERROR(parser_next_token(p));

        switch (p->current_token.type) {
        case TYPEOF:
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

            TRY_DO(type_expression_create(start_token, EXPLICIT, explicit_union, type, allocator), {
                NODE_VIRTUAL_FREE(referential_type, allocator);
                array_list_deinit(&dim_array);
            });
            break;
        case FUNCTION:
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
                        free_expression_list(&generics, allocator);
                        free_parameter_list(&parameters, allocator);
                        NODE_VIRTUAL_FREE(return_type, allocator);
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

            TRY_DO(type_expression_create(start_token, EXPLICIT, explicit_union, type, allocator), {
                free_expression_list(&generics, allocator);
                free_parameter_list(&parameters, allocator);
                NODE_VIRTUAL_FREE(return_type, allocator);
                array_list_deinit(&dim_array);
            });
            break;
        case STRUCT:
            Expression* struct_expr;
            TRY_DO(struct_expression_parse(p, &struct_expr), array_list_deinit(&dim_array));
            StructExpression* struct_type = (StructExpression*)struct_expr;

            explicit_union.explicit_type.tag     = EXPLICIT_STRUCT;
            explicit_union.explicit_type.variant = (ExplicitTypeUnion){
                .struct_type = struct_type,
            };

            TRY_DO(type_expression_create(start_token, EXPLICIT, explicit_union, type, allocator), {
                NODE_VIRTUAL_FREE(struct_type, allocator);
                array_list_deinit(&dim_array);
            });
            break;
        case ENUM:
            Expression* enum_expr;
            TRY_DO(enum_expression_parse(p, &enum_expr), array_list_deinit(&dim_array));
            EnumExpression* enum_type = (EnumExpression*)enum_expr;

            explicit_union.explicit_type.tag     = EXPLICIT_ENUM;
            explicit_union.explicit_type.variant = (ExplicitTypeUnion){
                .enum_type = enum_type,
            };

            TRY_DO(type_expression_create(start_token, EXPLICIT, explicit_union, type, allocator), {
                NODE_VIRTUAL_FREE(enum_type, allocator);
                array_list_deinit(&dim_array);
            });
            break;
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
        TRY_DO(type_expression_create(start_token, EXPLICIT, array_type_union, type, allocator), {
            array_list_deinit(&dim_array);
            NODE_VIRTUAL_FREE(*type, allocator);
        });
    }

    return SUCCESS;
}

[[nodiscard]] Status type_expression_parse(Parser* p, Expression** expression, bool* initialized) {
    Allocator*  allocator              = parser_allocator(p);
    const Token start_token            = p->current_token;
    bool        is_default_initialized = false;

    TypeExpression* type;
    if (parser_peek_token_is(p, WALRUS)) {
        TRY(type_expression_create(start_token, IMPLICIT, IMPLICIT_TYPE, &type, allocator));

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
    typedef T I;                                                                  \
    I*        integer;                                                            \
    TRY(create_fn(start_token, value, &integer, allocator));                      \
    *expression = (Expression*)integer;

[[nodiscard]] Status integer_literal_expression_parse(Parser* p, Expression** expression) {
    Allocator*  allocator   = parser_allocator(p);
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

[[nodiscard]] Status byte_literal_expression_parse(Parser* p, Expression** expression) {
    Allocator*  allocator   = parser_allocator(p);
    const Token start_token = p->current_token;
    uint8_t     value;
    TRY(strntochr(start_token.slice.ptr, start_token.slice.length, &value));

    ByteLiteralExpression* byte;
    TRY(byte_literal_expression_create(start_token, value, &byte, allocator));

    *expression = (Expression*)byte;
    return SUCCESS;
}

[[nodiscard]] Status float_literal_expression_parse(Parser* p, Expression** expression) {
    Allocator*  allocator   = parser_allocator(p);
    const Token start_token = p->current_token;
    assert(start_token.slice.length > 0);

    double       value;
    const Status parse_status = strntod(start_token.slice.ptr, start_token.slice.length, &value);
    if (STATUS_ERR(parse_status)) {
        PUT_STATUS_PROPAGATE(&p->errors, parse_status, start_token, {});
    }

    FloatLiteralExpression* float_expr;
    TRY(float_literal_expression_create(start_token, value, &float_expr, allocator));

    *expression = (Expression*)float_expr;
    return SUCCESS;
}

[[nodiscard]] Status prefix_expression_parse(Parser* p, Expression** expression) {
    Allocator*  allocator    = parser_allocator(p);
    const Token prefix_token = p->current_token;

    if (p->peek_token.type == END) {
        PUT_STATUS_PROPAGATE(&p->errors, PREFIX_MISSING_OPERAND, p->current_token, {});
    }
    TRY(parser_next_token(p));

    Expression* rhs;
    TRY(expression_parse(p, PREFIX, &rhs));

    PrefixExpression* prefix;
    TRY_DO(prefix_expression_create(prefix_token, rhs, &prefix, allocator),
           NODE_VIRTUAL_FREE(rhs, allocator));

    *expression = (Expression*)prefix;
    return SUCCESS;
}

[[nodiscard]] Status infix_expression_parse(Parser* p, Expression* left, Expression** expression) {
    Allocator*       allocator          = parser_allocator(p);
    const TokenType  op_token_type      = p->current_token.type;
    const Precedence current_precedence = parser_current_precedence(p);

    TRY(parser_next_token(p));
    ASSERT_EXPRESSION(left);

    Expression* right;
    TRY(expression_parse(p, current_precedence, &right));

    Node*            left_node = (Node*)left;
    InfixExpression* infix;
    TRY_DO(infix_expression_create(
               left_node->start_token, left, op_token_type, right, &infix, allocator),
           NODE_VIRTUAL_FREE(right, allocator));

    *expression = (Expression*)infix;
    return SUCCESS;
}

[[nodiscard]] Status bool_expression_parse(Parser* p, Expression** expression) {
    Allocator*             allocator = parser_allocator(p);
    BoolLiteralExpression* boolean;
    TRY(bool_literal_expression_create(p->current_token, &boolean, allocator));
    *expression = (Expression*)boolean;
    return SUCCESS;
}

[[nodiscard]] Status string_expression_parse(Parser* p, Expression** expression) {
    Allocator*               allocator = parser_allocator(p);
    StringLiteralExpression* string;
    TRY(string_literal_expression_create(p->current_token, &string, allocator));
    *expression = (Expression*)string;
    return SUCCESS;
}

[[nodiscard]] Status grouped_expression_parse(Parser* p, Expression** expression) {
    Allocator* allocator = parser_allocator(p);
    UNREACHABLE_IF_ERROR(parser_next_token(p));

    Expression* inner;
    TRY(expression_parse(p, LOWEST, &inner));

    TRY_DO(parser_expect_peek(p, RPAREN), NODE_VIRTUAL_FREE(inner, allocator));

    *expression = inner;
    return SUCCESS;
}

[[nodiscard]] static inline Status if_expression_parse_branch(Parser* p, Statement** stmt) {
    TRY(parser_next_token(p));

    Statement* alternate;
    TRAILING_ALTERNATE(alternate, {}, ILLEGAL_IF_BRANCH);
    *stmt = alternate;
    return SUCCESS;
}

[[nodiscard]] Status if_expression_parse(Parser* p, Expression** expression) {
    Allocator*  allocator   = parser_allocator(p);
    const Token start_token = p->current_token;

    TRY(parser_expect_peek(p, LPAREN));
    TRY(parser_next_token(p));

    Expression* condition;
    TRY(expression_parse(p, LOWEST, &condition));

    TRY_DO(parser_expect_peek(p, RPAREN), NODE_VIRTUAL_FREE(condition, allocator));

    Statement* consequence;
    TRY_DO(if_expression_parse_branch(p, &consequence), NODE_VIRTUAL_FREE(condition, allocator));

    Statement* alternate = nullptr;
    if (parser_peek_token_is(p, ELSE)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
        TRY_DO(if_expression_parse_branch(p, &alternate), {
            NODE_VIRTUAL_FREE(condition, allocator);
            NODE_VIRTUAL_FREE(consequence, allocator);
        });
    }

    IfExpression* if_expr;
    TRY_DO(
        if_expression_create(start_token, condition, consequence, alternate, &if_expr, allocator), {
            NODE_VIRTUAL_FREE(condition, allocator);
            NODE_VIRTUAL_FREE(consequence, allocator);
            NODE_VIRTUAL_FREE(alternate, allocator);
        });

    *expression = (Expression*)if_expr;
    return SUCCESS;
}

[[nodiscard]] Status function_expression_parse(Parser* p, Expression** expression) {
    Allocator*  allocator   = parser_allocator(p);
    const Token start_token = p->current_token;

    ArrayList       generics;
    ArrayList       parameters;
    TypeExpression* return_type;

    TRY(function_definition_parse(p, &generics, &parameters, &return_type, nullptr));
    TRY_DO(parser_expect_peek(p, LBRACE), {
        free_expression_list(&generics, allocator);
        free_parameter_list(&parameters, allocator);
        NODE_VIRTUAL_FREE(return_type, allocator);
    });

    BlockStatement* body;
    TRY_DO(block_statement_parse(p, &body), {
        free_expression_list(&generics, allocator);
        free_parameter_list(&parameters, allocator);
        NODE_VIRTUAL_FREE(return_type, allocator);
    });

    FunctionExpression* function;
    TRY_DO(function_expression_create(
               start_token, generics, parameters, return_type, body, &function, allocator),
           {
               free_expression_list(&generics, allocator);
               NODE_VIRTUAL_FREE(return_type, allocator);
               free_parameter_list(&parameters, allocator);
               NODE_VIRTUAL_FREE(body, allocator);
           });

    *expression = (Expression*)function;
    return SUCCESS;
}

[[nodiscard]] Status
call_expression_parse(Parser* p, Expression* function, Expression** expression) {
    Allocator*  allocator   = parser_allocator(p);
    const Token start_token = p->current_token;

    ArrayList arguments;
    TRY(array_list_init_allocator(&arguments, 2, sizeof(CallArgument), allocator));

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
            NODE_VIRTUAL_FREE(argument_expr, allocator);
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

            TRY_DO(parser_next_token(p), free_call_expression_list(&arguments, allocator));

            TRY_DO(expression_parse(p, LOWEST, &argument_expr),
                   free_call_expression_list(&arguments, allocator));

            argument = (CallArgument){.is_ref = is_ref_argument, .argument = argument_expr};
            TRY_DO(array_list_push(&arguments, &argument),
                   free_call_expression_list(&arguments, allocator));
        }

        TRY_DO(parser_expect_peek(p, RPAREN), free_call_expression_list(&arguments, allocator));
    }

    // Since call expressions are infixed, their generics are at the end of the expression
    ArrayList generics;
    if (parser_peek_token_is(p, WITH)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
        TRY_DO(generics_parse(p, &generics), free_call_expression_list(&arguments, allocator));
    } else {
        TRY(array_list_init_allocator(&generics, 1, sizeof(Expression*), allocator));
    }

    // We can just move into the call expression now since generics is necessary allocated
    CallExpression* call;
    TRY_DO(call_expression_create(start_token, function, arguments, generics, &call, allocator), {
        free_call_expression_list(&arguments, allocator);
        free_expression_list(&generics, allocator);
    });

    *expression = (Expression*)call;
    return SUCCESS;
}

[[nodiscard]] Status index_expression_parse(Parser* p, Expression* array, Expression** expression) {
    ASSERT_EXPRESSION(array);
    Allocator*  allocator   = parser_allocator(p);
    const Token start_token = p->current_token;

    if (parser_peek_token_is(p, RBRACE)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
        PUT_STATUS_PROPAGATE(&p->errors, INDEX_MISSING_EXPRESSION, start_token, {});
    }

    TRY(parser_next_token(p));
    Expression* idx_expr;
    TRY(expression_parse(p, LOWEST, &idx_expr));

    IndexExpression* index;
    TRY_DO(index_expression_create(start_token, array, idx_expr, &index, allocator),
           NODE_VIRTUAL_FREE(idx_expr, allocator));

    TRY_DO(parser_expect_peek(p, RBRACKET), NODE_VIRTUAL_FREE(index, allocator));

    *expression = (Expression*)index;
    return SUCCESS;
}

[[nodiscard]] Status struct_expression_parse(Parser* p, Expression** expression) {
    Allocator*  allocator   = parser_allocator(p);
    const Token start_token = p->current_token;
    ArrayList   generics;
    TRY(generics_parse(p, &generics));

    TRY_DO(parser_expect_peek(p, LBRACE), free_expression_list(&generics, allocator));
    if (parser_peek_token_is(p, RBRACE)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
        PUT_STATUS_PROPAGATE(&p->errors,
                             STRUCT_MISSING_MEMBERS,
                             start_token,
                             free_expression_list(&generics, allocator));
    }

    ArrayList members;
    TRY_DO(array_list_init_allocator(&members, 4, sizeof(StructMember), allocator),
           free_expression_list(&generics, allocator));

    // We only handle members here as methods are in impl blocks
    while (!parser_peek_token_is(p, RBRACE) && !parser_peek_token_is(p, END)) {
        TRY_DO(parser_expect_peek(p, IDENT), {
            free_struct_member_list(&members, allocator);
            free_expression_list(&generics, allocator);
        });

        Expression* ident_expr;
        TRY_DO(identifier_expression_parse(p, &ident_expr), {
            free_struct_member_list(&members, allocator);
            free_expression_list(&generics, allocator);
        });

        Expression* type_expr;
        bool        has_default_value;
        TRY_DO(type_expression_parse(p, &type_expr, &has_default_value), {
            NODE_VIRTUAL_FREE(ident_expr, allocator);
            free_struct_member_list(&members, allocator);
            free_expression_list(&generics, allocator);
        });

        // Struct members must have explicit type declarations
        TypeExpression* type = (TypeExpression*)type_expr;
        if (type->tag == IMPLICIT) {
            PUT_STATUS_PROPAGATE(&p->errors, STRUCT_MEMBER_NOT_EXPLICIT, p->current_token, {
                NODE_VIRTUAL_FREE(ident_expr, allocator);
                NODE_VIRTUAL_FREE(type_expr, allocator);
                free_struct_member_list(&members, allocator);
                free_expression_list(&generics, allocator);
            });
        }

        // The type parser leaves with the current token on assignment in this case
        Expression* default_value = nullptr;
        if (has_default_value) {
            TRY_DO(expression_parse(p, LOWEST, &default_value), {
                NODE_VIRTUAL_FREE(ident_expr, allocator);
                NODE_VIRTUAL_FREE(type_expr, allocator);
                free_struct_member_list(&members, allocator);
                free_expression_list(&generics, allocator);
            });
        }

        // Now we can create the member and store its data pointers
        const StructMember member = (StructMember){
            .name          = (IdentifierExpression*)ident_expr,
            .type          = type,
            .default_value = default_value,
        };

        TRY_DO(array_list_push(&members, &member), {
            NODE_VIRTUAL_FREE(ident_expr, allocator);
            NODE_VIRTUAL_FREE(type_expr, allocator);
            NODE_VIRTUAL_FREE(default_value, allocator);
            free_struct_member_list(&members, allocator);
            free_expression_list(&generics, allocator);
        });

        // All members require a trailing comma!
        if (has_default_value && parser_peek_token_is(p, COMMA)) {
            UNREACHABLE_IF_ERROR(parser_next_token(p));
        } else if (!parser_current_token_is(p, COMMA)) {
            PUT_STATUS_PROPAGATE(&p->errors, MISSING_TRAILING_COMMA, p->current_token, {
                free_struct_member_list(&members, allocator);
                free_expression_list(&generics, allocator);
            });
        }
    }

    TRY_DO(parser_expect_peek(p, RBRACE), {
        free_struct_member_list(&members, allocator);
        free_expression_list(&generics, allocator);
    });

    StructExpression* struct_expr;
    TRY_DO(struct_expression_create(start_token, generics, members, &struct_expr, allocator), {
        free_struct_member_list(&members, allocator);
        free_expression_list(&generics, allocator);
    });

    *expression = (Expression*)struct_expr;
    return SUCCESS;
}

[[nodiscard]] Status enum_expression_parse(Parser* p, Expression** expression) {
    Allocator*  allocator   = parser_allocator(p);
    const Token start_token = p->current_token;

    TRY(parser_expect_peek(p, LBRACE));
    if (parser_peek_token_is(p, RBRACE)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
        PUT_STATUS_PROPAGATE(&p->errors, ENUM_MISSING_VARIANTS, start_token, {});
    }

    ArrayList variants;
    TRY(array_list_init_allocator(&variants, 4, sizeof(EnumVariant), allocator));
    while (!parser_peek_token_is(p, RBRACE) && !parser_peek_token_is(p, END)) {
        TRY_DO(parser_expect_peek(p, IDENT), free_enum_variant_list(&variants, allocator));

        Expression* ident_expr;
        TRY_DO(identifier_expression_parse(p, &ident_expr),
               free_enum_variant_list(&variants, allocator));
        IdentifierExpression* ident = (IdentifierExpression*)ident_expr;

        Expression* value = nullptr;
        if (parser_peek_token_is(p, ASSIGN)) {
            UNREACHABLE_IF_ERROR(parser_next_token(p));
            TRY_DO(parser_next_token(p), {
                free_enum_variant_list(&variants, allocator);
                NODE_VIRTUAL_FREE(ident, allocator);
            });

            TRY_DO(expression_parse(p, LOWEST, &value), {
                free_enum_variant_list(&variants, allocator);
                NODE_VIRTUAL_FREE(ident, allocator);
            });
        }

        EnumVariant variant = (EnumVariant){.name = ident, .value = value};
        TRY_DO(array_list_push(&variants, &variant), {
            free_enum_variant_list(&variants, allocator);
            NODE_VIRTUAL_FREE(ident, allocator);
            NODE_VIRTUAL_FREE(value, allocator);
        });

        // All variants require a trailing comma!
        if (STATUS_ERR(parser_expect_peek(p, COMMA))) {
            PUT_STATUS_PROPAGATE(&p->errors,
                                 MISSING_TRAILING_COMMA,
                                 p->current_token,
                                 free_enum_variant_list(&variants, allocator));
        }
    }

    TRY_DO(parser_expect_peek(p, RBRACE), free_enum_variant_list(&variants, allocator));

    EnumExpression* enum_expr;
    TRY_DO(enum_expression_create(start_token, variants, &enum_expr, allocator),
           free_enum_variant_list(&variants, allocator));

    *expression = (Expression*)enum_expr;
    return SUCCESS;
}

[[nodiscard]] Status nil_expression_parse(Parser* p, Expression** expression) {
    Allocator*     allocator = parser_allocator(p);
    NilExpression* nil;
    TRY(nil_expression_create(p->current_token, &nil, allocator));
    *expression = (Expression*)nil;
    return SUCCESS;
}

[[nodiscard]] Status match_expression_parse(Parser* p, Expression** expression) {
    Allocator*  allocator   = parser_allocator(p);
    const Token start_token = p->current_token;

    TRY(parser_next_token(p));

    Expression* match_cond_expr;
    TRY(expression_parse(p, LOWEST, &match_cond_expr));
    TRY_DO(parser_expect_peek(p, LBRACE), NODE_VIRTUAL_FREE(match_cond_expr, allocator));

    if (parser_peek_token_is(p, RBRACE)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
        PUT_STATUS_PROPAGATE(&p->errors,
                             ARMLESS_MATCH_EXPR,
                             start_token,
                             NODE_VIRTUAL_FREE(match_cond_expr, allocator));
    }

    ArrayList arms;
    TRY_DO(array_list_init_allocator(&arms, 4, sizeof(MatchArm), allocator),
           NODE_VIRTUAL_FREE(match_cond_expr, allocator));
    while (!parser_peek_token_is(p, RBRACE) && !parser_peek_token_is(p, END)) {
        // Current token which is either the LBRACE at the start or a comma before parsing
        UNREACHABLE_IF_ERROR(parser_next_token(p));

        Expression* pattern;
        TRY_DO(expression_parse(p, LOWEST, &pattern),
               NODE_VIRTUAL_FREE(match_cond_expr, allocator));
        TRY_DO(parser_expect_peek(p, FAT_ARROW), {
            NODE_VIRTUAL_FREE(match_cond_expr, allocator);
            NODE_VIRTUAL_FREE(pattern, allocator);
            free_match_arm_list(&arms, allocator);
        });

        // Guard the statement from being global/declaration based
        TRY_DO(parser_next_token(p), {
            NODE_VIRTUAL_FREE(match_cond_expr, allocator);
            NODE_VIRTUAL_FREE(pattern, allocator);
            free_match_arm_list(&arms, allocator);
        });

        Statement* consequence;
        TRAILING_ALTERNATE(
            consequence,
            {
                NODE_VIRTUAL_FREE(match_cond_expr, allocator);
                NODE_VIRTUAL_FREE(pattern, allocator);
                free_match_arm_list(&arms, allocator);
            },
            ILLEGAL_MATCH_ARM);

        // As per the rest of the language, commas are required as trailing tokens
        TRY_DO(parser_expect_peek(p, COMMA), {
            NODE_VIRTUAL_FREE(match_cond_expr, allocator);
            NODE_VIRTUAL_FREE(pattern, allocator);
            NODE_VIRTUAL_FREE(consequence, allocator);
            free_match_arm_list(&arms, allocator);
        });

        MatchArm arm = (MatchArm){.pattern = pattern, .dispatch = consequence};
        TRY_DO(array_list_push(&arms, &arm), {
            NODE_VIRTUAL_FREE(match_cond_expr, allocator);
            NODE_VIRTUAL_FREE(pattern, allocator);
            NODE_VIRTUAL_FREE(consequence, allocator);
            free_match_arm_list(&arms, allocator);
        });
    }

    // Empty match statements aren't ever allowed
    if (arms.length == 0) {
        PUT_STATUS_PROPAGATE(&p->errors, ARMLESS_MATCH_EXPR, start_token, {
            NODE_VIRTUAL_FREE(match_cond_expr, allocator);
            free_match_arm_list(&arms, allocator);
        });
    }

    TRY_DO(parser_expect_peek(p, RBRACE), {
        NODE_VIRTUAL_FREE(match_cond_expr, allocator);
        free_match_arm_list(&arms, allocator);
    });

    // Catch all statement is optional and is the equivalent of a switch default
    Statement* catch_all = nullptr;
    POSSIBLE_TRAILING_ALTERNATE(
        catch_all,
        {
            NODE_VIRTUAL_FREE(match_cond_expr, allocator);
            free_match_arm_list(&arms, allocator);
        },
        ILLEGAL_MATCH_CATCH_ALL);

    MatchExpression* match;
    TRY_DO(
        match_expression_create(start_token, match_cond_expr, arms, catch_all, &match, allocator), {
            NODE_VIRTUAL_FREE(match_cond_expr, allocator);
            free_match_arm_list(&arms, allocator);
        });

    *expression = (Expression*)match;
    return SUCCESS;
}

[[nodiscard]] Status array_literal_expression_parse(Parser* p, Expression** expression) {
    Allocator*  allocator   = parser_allocator(p);
    const Token start_token = p->current_token;
    TRY(parser_next_token(p));

    bool   inferred_size;
    size_t array_size;
    if (p->current_token.type == UNDERSCORE) {
        array_size    = 0;
        inferred_size = true;
    } else {
        const Token integer_token = p->current_token;
        inferred_size             = false;

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
        &items, array_size == 0 ? 4 : array_size, sizeof(Expression*), allocator));

    while (!parser_peek_token_is(p, RBRACE) && !parser_peek_token_is(p, END)) {
        // Current token which is either the LBRACE at the start or a comma before parsing
        UNREACHABLE_IF_ERROR(parser_next_token(p));

        Expression* item;
        TRY_DO(expression_parse(p, LOWEST, &item), free_expression_list(&items, allocator));

        // Add to the array list and expect a comma to align with language philosophy
        TRY_DO(array_list_push(&items, (const void*)&item), {
            NODE_VIRTUAL_FREE(item, allocator);
            free_expression_list(&items, allocator);
        });

        TRY_DO(parser_expect_peek(p, COMMA), free_expression_list(&items, allocator));
    }

    TRY_DO(parser_expect_peek(p, RBRACE), free_expression_list(&items, allocator));
    if (!inferred_size && items.length != array_size) {
        PUT_STATUS_PROPAGATE(&p->errors,
                             INCORRECT_EXPLICIT_ARRAY_SIZE,
                             start_token,
                             free_expression_list(&items, allocator));
    }

    if (items.length == 0 || (!inferred_size && array_size == 0)) {
        PUT_STATUS_PROPAGATE(
            &p->errors, EMPTY_ARRAY, start_token, free_expression_list(&items, allocator));
    }

    ArrayLiteralExpression* array;
    TRY_DO(array_literal_expression_create(start_token, inferred_size, items, &array, allocator),
           free_expression_list(&items, allocator));

    *expression = (Expression*)array;
    return SUCCESS;
}

[[nodiscard]] Status for_loop_expression_parse(Parser* p, Expression** expression) {
    Allocator*  allocator   = parser_allocator(p);
    const Token start_token = p->current_token;
    TRY(parser_expect_peek(p, LPAREN));

    ArrayList iterables;
    TRY(array_list_init_allocator(&iterables, 2, sizeof(Expression*), allocator));
    while (!parser_peek_token_is(p, RPAREN) && !parser_peek_token_is(p, END)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));

        Expression* iterable;
        TRY_DO(expression_parse(p, LOWEST, &iterable), free_expression_list(&iterables, allocator));

        TRY_DO(array_list_push(&iterables, (const void*)&iterable),
               free_expression_list(&iterables, allocator));

        // Only check for the comma after confirming we aren't at the end of the iterables
        if (!parser_peek_token_is(p, RPAREN)) {
            TRY_DO(parser_expect_peek(p, COMMA), free_expression_list(&iterables, allocator));
        }
    }

    TRY_DO(parser_expect_peek(p, RPAREN), free_expression_list(&iterables, allocator));

    if (iterables.length == 0) {
        PUT_STATUS_PROPAGATE(&p->errors,
                             FOR_MISSING_ITERABLES,
                             start_token,
                             free_expression_list(&iterables, allocator));
    }

    // Captures are technically optional, but they are allocated anyways
    ArrayList captures;
    TRY(array_list_init_allocator(&captures, 2, sizeof(ForLoopCapture), allocator));

    bool expect_captures = false;
    if (parser_peek_token_is(p, COLON)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
        TRY_DO(parser_expect_peek(p, LPAREN), {
            free_expression_list(&iterables, allocator);
            free_for_capture_list(&captures, allocator);
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
                TRY_DO(ignore_expression_create(p->current_token, &ignore, allocator), {
                    free_expression_list(&iterables, allocator);
                    free_for_capture_list(&captures, allocator);
                });
                capture = (ForLoopCapture){.is_ref = false, .capture = (Expression*)ignore};
            } else {
                Expression* capture_expr;
                TRY_DO(expression_parse(p, LOWEST, &capture_expr), {
                    free_expression_list(&iterables, allocator);
                    free_for_capture_list(&captures, allocator);
                });
                capture = (ForLoopCapture){.is_ref = is_ref_argument, .capture = capture_expr};
            }

            TRY_DO(array_list_push(&captures, &capture), {
                free_expression_list(&iterables, allocator);
                free_for_capture_list(&captures, allocator);
            });

            // Only check for the close bar if we have more captures
            if (!parser_peek_token_is(p, RPAREN)) {
                TRY_DO(parser_expect_peek(p, COMMA), {
                    free_expression_list(&iterables, allocator);
                    free_for_capture_list(&captures, allocator);
                });
            }
        }

        TRY_DO(parser_expect_peek(p, RPAREN), {
            free_expression_list(&iterables, allocator);
            free_for_capture_list(&captures, allocator);
        });

        expect_captures = true;
    }

    // The number of captures must align with the number of iterables
    if (expect_captures && iterables.length != captures.length) {
        PUT_STATUS_PROPAGATE(&p->errors, FOR_ITERABLE_CAPTURE_MISMATCH, start_token, {
            free_expression_list(&iterables, allocator);
            free_for_capture_list(&captures, allocator);
        });
    }

    // Now we can parse the block statement as usual
    TRY_DO(parser_expect_peek(p, LBRACE), {
        free_expression_list(&iterables, allocator);
        free_for_capture_list(&captures, allocator);
    });
    BlockStatement* block;
    TRY_DO(block_statement_parse(p, &block), {
        free_expression_list(&iterables, allocator);
        free_for_capture_list(&captures, allocator);
    });

    // There is nothing that can be done in an empty for loop so we prevent it here
    if (block->statements.length == 0) {
        PUT_STATUS_PROPAGATE(&p->errors, EMPTY_FOR_LOOP, start_token, {
            free_expression_list(&iterables, allocator);
            free_for_capture_list(&captures, allocator);
            NODE_VIRTUAL_FREE(block, allocator);
        });
    }

    // Finally, we should try to parse the non break clause
    Statement* non_break = nullptr;
    POSSIBLE_TRAILING_ALTERNATE(
        non_break,
        {
            free_expression_list(&iterables, allocator);
            free_for_capture_list(&captures, allocator);
            NODE_VIRTUAL_FREE(block, allocator);
        },
        ILLEGAL_LOOP_NON_BREAK);

    // We're now ready to construct the actual object and transfer ownership
    ForLoopExpression* for_loop;
    TRY_DO(for_loop_expression_create(
               start_token, iterables, captures, block, non_break, &for_loop, allocator),
           {
               free_expression_list(&iterables, allocator);
               free_for_capture_list(&captures, allocator);
               NODE_VIRTUAL_FREE(block, allocator);
               NODE_VIRTUAL_FREE(non_break, allocator);
           });

    *expression = (Expression*)for_loop;
    return SUCCESS;
}

[[nodiscard]] Status while_loop_expression_parse(Parser* p, Expression** expression) {
    Allocator*  allocator   = parser_allocator(p);
    const Token start_token = p->current_token;

    TRY(parser_expect_peek(p, LPAREN));
    TRY(parser_next_token(p));

    Expression* condition;
    if (parser_current_token_is(p, RPAREN)) {
        PUT_STATUS_PROPAGATE(&p->errors, WHILE_MISSING_CONDITION, p->current_token, {});
    }
    TRY(expression_parse(p, LOWEST, &condition));

    TRY_DO(parser_expect_peek(p, RPAREN), NODE_VIRTUAL_FREE(condition, allocator));

    // The continuation expression is optional but is parsed similar to captures
    Expression* continuation = nullptr;
    if (parser_peek_token_is(p, COLON)) {
        const Token continuation_start = p->current_token;
        UNREACHABLE_IF_ERROR(parser_next_token(p));
        TRY_DO(parser_expect_peek(p, LPAREN), NODE_VIRTUAL_FREE(condition, allocator));

        // We have to consume again since we're looking at the LPAREN here
        TRY_DO(parser_next_token(p), NODE_VIRTUAL_FREE(condition, allocator));
        if (parser_current_token_is(p, RPAREN)) {
            PUT_STATUS_PROPAGATE(&p->errors,
                                 IMPROPER_WHILE_CONTINUATION,
                                 continuation_start,
                                 NODE_VIRTUAL_FREE(condition, allocator));
        }
        TRY_DO(expression_parse(p, LOWEST, &continuation), NODE_VIRTUAL_FREE(condition, allocator));

        TRY_DO(parser_expect_peek(p, RPAREN), {
            NODE_VIRTUAL_FREE(condition, allocator);
            NODE_VIRTUAL_FREE(continuation, allocator);
        });
    }

    // Now we can parse the block statement now since it has to be here
    TRY_DO(parser_expect_peek(p, LBRACE), {
        NODE_VIRTUAL_FREE(condition, allocator);
        NODE_VIRTUAL_FREE(continuation, allocator);
    });

    BlockStatement* block;
    TRY_DO(block_statement_parse(p, &block), {
        NODE_VIRTUAL_FREE(condition, allocator);
        NODE_VIRTUAL_FREE(continuation, allocator);
    });

    // We need at least a continuation or block
    if (!continuation && block->statements.length == 0) {
        PUT_STATUS_PROPAGATE(&p->errors, EMPTY_WHILE_LOOP, start_token, {
            NODE_VIRTUAL_FREE(condition, allocator);
            NODE_VIRTUAL_FREE(continuation, allocator);
            NODE_VIRTUAL_FREE(block, allocator);
        });
    }

    // Finally, we should try to parse the non break clause
    Statement* non_break = nullptr;
    POSSIBLE_TRAILING_ALTERNATE(
        non_break,
        {
            NODE_VIRTUAL_FREE(condition, allocator);
            NODE_VIRTUAL_FREE(continuation, allocator);
            NODE_VIRTUAL_FREE(block, allocator);
        },
        ILLEGAL_LOOP_NON_BREAK);

    // Ownership transfer time
    WhileLoopExpression* while_loop;
    TRY_DO(while_loop_expression_create(
               start_token, condition, continuation, block, non_break, &while_loop, allocator),
           {
               NODE_VIRTUAL_FREE(condition, allocator);
               NODE_VIRTUAL_FREE(continuation, allocator);
               NODE_VIRTUAL_FREE(block, allocator);
           });

    *expression = (Expression*)while_loop;
    return SUCCESS;
}

[[nodiscard]] Status do_while_loop_expression_parse(Parser* p, Expression** expression) {
    Allocator*  allocator   = parser_allocator(p);
    const Token start_token = p->current_token;
    TRY(parser_expect_peek(p, LBRACE));

    // Parse the block statement which must do some work
    BlockStatement* block;
    TRY(block_statement_parse(p, &block));

    if (block->statements.length == 0) {
        PUT_STATUS_PROPAGATE(
            &p->errors, EMPTY_WHILE_LOOP, start_token, NODE_VIRTUAL_FREE(block, allocator));
    }

    // We have to consume the while token, the open paren, and enter the expression
    TRY_DO(parser_expect_peek(p, WHILE), NODE_VIRTUAL_FREE(block, allocator));
    TRY_DO(parser_expect_peek(p, LPAREN), NODE_VIRTUAL_FREE(block, allocator));
    TRY_DO(parser_next_token(p), NODE_VIRTUAL_FREE(block, allocator));

    // There's no continuation or non break clause so this is easy :)
    Expression* condition;
    if (parser_current_token_is(p, RPAREN)) {
        PUT_STATUS_PROPAGATE(&p->errors,
                             WHILE_MISSING_CONDITION,
                             p->current_token,
                             NODE_VIRTUAL_FREE(block, allocator));
    }
    TRY_DO(expression_parse(p, LOWEST, &condition), NODE_VIRTUAL_FREE(block, allocator));

    // Ownership transfer time
    DoWhileLoopExpression* do_while_loop;
    TRY_DO(
        do_while_loop_expression_create(start_token, block, condition, &do_while_loop, allocator), {
            NODE_VIRTUAL_FREE(block, allocator);
            NODE_VIRTUAL_FREE(condition, allocator);
        });

    TRY_DO(parser_expect_peek(p, RPAREN), NODE_VIRTUAL_FREE(do_while_loop, allocator));

    *expression = (Expression*)do_while_loop;
    return SUCCESS;
}

[[nodiscard]] Status loop_expression_parse(Parser* p, Expression** expression) {
    Allocator*  allocator   = parser_allocator(p);
    const Token start_token = p->current_token;
    TRY(parser_expect_peek(p, LBRACE));

    // Parse the block statement which must do some work
    BlockStatement* block;
    TRY(block_statement_parse(p, &block));

    if (block->statements.length == 0) {
        PUT_STATUS_PROPAGATE(
            &p->errors, EMPTY_LOOP, start_token, NODE_VIRTUAL_FREE(block, allocator));
    }

    // Super simple parsing here since this is just a raw loop
    LoopExpression* loop;
    TRY_DO(loop_expression_create(start_token, block, &loop, allocator),
           NODE_VIRTUAL_FREE(block, allocator));

    *expression = (Expression*)loop;
    return SUCCESS;
}

[[nodiscard]] Status
namespace_expression_parse(Parser* p, Expression* outer, Expression** expression) {
    ASSERT_EXPRESSION(outer);
    Allocator* allocator = parser_allocator(p);
    TRY(parser_expect_peek(p, IDENT));

    Expression* ident_expr;
    TRY(identifier_expression_parse(p, &ident_expr));
    IdentifierExpression* ident = (IdentifierExpression*)ident_expr;

    Node*                outer_node = (Node*)outer;
    NamespaceExpression* namespace_expr;
    TRY_DO(namespace_expression_create(
               outer_node->start_token, outer, ident, &namespace_expr, allocator),
           NODE_VIRTUAL_FREE(ident, allocator));

    *expression = (Expression*)namespace_expr;
    return SUCCESS;
}

[[nodiscard]] Status
assignment_expression_parse(Parser* p, Expression* assignee, Expression** expression) {
    Allocator*       allocator          = parser_allocator(p);
    const TokenType  op_token_type      = p->current_token.type;
    const Precedence current_precedence = parser_current_precedence(p);

    TRY(parser_next_token(p));
    ASSERT_EXPRESSION(assignee);

    Expression* right;
    TRY(expression_parse(p, current_precedence, &right));

    AssignmentExpression* assign;
    TRY_DO(assignment_expression_create(
               NODE_TOKEN(assignee), assignee, op_token_type, right, &assign, allocator),
           NODE_VIRTUAL_FREE(right, allocator));

    *expression = (Expression*)assign;
    return SUCCESS;
}
