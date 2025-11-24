#include <assert.h>
#include <stdio.h>

#include "lexer/token.h"

#include "ast/ast.h"
#include "ast/expressions/bool.h"
#include "ast/expressions/expression.h"
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
#include "ast/statements/return.h"
#include "ast/statements/statement.h"

#include "parser/expression_parsers.h"
#include "parser/parser.h"
#include "parser/statement_parsers.h"

#include "util/allocator.h"
#include "util/alphanum.h"
#include "util/containers/hash_set.h"
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
    PROPAGATE_IF_ERROR_DO(string_builder_append_many(&sb, token_literal, strlen(token_literal)),
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

TRY_STATUS type_expression_parse(Parser* p, Expression** expression, bool* initialized) {
    assert(p->primitives.buffer);
    *initialized = false;

    TypeExpression* type;
    if (parser_peek_token_is(p, WALRUS)) {
        PROPAGATE_IF_ERROR(
            type_expression_create(IMPLICIT, IMPLICIT_TYPE, &type, p->allocator.memory_alloc));

        UNREACHABLE_IF_ERROR(parser_next_token(p));
        *initialized = true;
    } else if (parser_peek_token_is(p, COLON)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));

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
                type_expression_create(EXPLICIT, explicit_union, &type, p->allocator.memory_alloc),
                {
                    Node* ident_node = (Node*)ident_expr;
                    ident_node->vtable->destroy(ident_node, p->allocator.free_alloc);
                });
        } else if (parser_peek_token_is(p, FUNCTION)) {
            const Token type_start = p->current_token;
            UNREACHABLE_IF_ERROR(parser_next_token(p));
            PROPAGATE_IF_ERROR(parser_expect_peek(p, LPAREN));

            bool      contains_default_param;
            ArrayList parameters;
            PROPAGATE_IF_ERROR(allocate_parameter_list(p, &parameters, &contains_default_param));

            // Default values in function types make no sense to allow, but shouldn't halt parsing
            if (contains_default_param) {
                PROPAGATE_IF_ERROR_DO(
                    parser_put_status_error(
                        p, ILLEGAL_DEFAULT_FUNCTION_PARAMETER, type_start.line, type_start.column),
                    free_parameter_list(&parameters));
            }

            explicit_union.explicit_type.tag     = EXPLICIT_FN;
            explicit_union.explicit_type.variant = (ExplicitTypeUnion){
                .fn_type_params = parameters,
            };

            PROPAGATE_IF_ERROR_DO(
                type_expression_create(EXPLICIT, explicit_union, &type, p->allocator.memory_alloc),
                free_parameter_list(&parameters));
        } else {
            return UNEXPECTED_TOKEN;
        }

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
    const Token current = p->current_token;
    assert(current.slice.length > 0);
    const int base = integer_token_to_base(current.type);

    if (token_is_signed_integer(current.type)) {
        int64_t      value;
        const Status parse_status = strntoll(current.slice.ptr, current.slice.length, base, &value);
        if (STATUS_ERR(parse_status)) {
            PROPAGATE_IF_ERROR(
                parser_put_status_error(p, parse_status, current.line, current.column));
            return parse_status;
        }

        IntegerLiteralExpression* integer;
        PROPAGATE_IF_ERROR(
            integer_literal_expression_create(current, value, &integer, p->allocator.memory_alloc));

        *expression = (Expression*)integer;
    } else if (token_is_unsigned_integer(current.type)) {
        uint64_t     value;
        const Status parse_status =
            strntoull(current.slice.ptr, current.slice.length - 1, base, &value);
        if (STATUS_ERR(parse_status)) {
            PROPAGATE_IF_ERROR(
                parser_put_status_error(p, parse_status, current.line, current.column));
            return parse_status;
        }

        UnsignedIntegerLiteralExpression* integer;
        PROPAGATE_IF_ERROR(uinteger_literal_expression_create(
            current, value, &integer, p->allocator.memory_alloc));

        *expression = (Expression*)integer;
    } else {
        return UNEXPECTED_TOKEN;
    }

    return SUCCESS;
}

TRY_STATUS float_literal_expression_parse(Parser* p, Expression** expression) {
    const Token current = p->current_token;
    assert(current.slice.length > 0);

    double       value;
    const Status parse_status = strntod(current.slice.ptr,
                                        current.slice.length,
                                        &value,
                                        p->allocator.memory_alloc,
                                        p->allocator.free_alloc);
    if (STATUS_ERR(parse_status)) {
        PROPAGATE_IF_ERROR(parser_put_status_error(p, parse_status, current.line, current.column));
        return parse_status;
    }

    FloatLiteralExpression* float_expr;
    PROPAGATE_IF_ERROR(
        float_literal_expression_create(current, value, &float_expr, p->allocator.memory_alloc));

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
        prefix_expression_create(prefix_token, rhs, &prefix, p->allocator.memory_alloc), {
            Node* node = (Node*)rhs;
            node->vtable->destroy(node, p->allocator.free_alloc);
        });

    *expression = (Expression*)prefix;
    return SUCCESS;
}

TRY_STATUS infix_expression_parse(Parser* p, Expression* left, Expression** expression) {
    const TokenType  op_token_type      = p->current_token.type;
    const Precedence current_precedence = parser_current_precedence(p);
    PROPAGATE_IF_ERROR(parser_next_token(p));

    Expression* right;
    PROPAGATE_IF_ERROR(expression_parse(p, current_precedence, &right));

    InfixExpression* infix;
    PROPAGATE_IF_ERROR_DO(
        infix_expression_create(left, op_token_type, right, &infix, p->allocator.memory_alloc), {
            Node* node = (Node*)right;
            node->vtable->destroy(node, p->allocator.free_alloc);
        });

    *expression = (Expression*)infix;
    return SUCCESS;
}

TRY_STATUS bool_expression_parse(Parser* p, Expression** expression) {
    BoolLiteralExpression* boolean;
    PROPAGATE_IF_ERROR(bool_literal_expression_create(
        p->current_token.type == TRUE, &boolean, p->allocator.memory_alloc));
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

    PROPAGATE_IF_ERROR_DO(parser_expect_peek(p, RPAREN), {
        Node* inner_node = (Node*)inner;
        inner_node->vtable->destroy(inner_node, p->allocator.free_alloc);
    });

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
    PROPAGATE_IF_ERROR(parser_expect_peek(p, LPAREN));
    PROPAGATE_IF_ERROR(parser_next_token(p));

    Expression* condition;
    PROPAGATE_IF_ERROR(expression_parse(p, LOWEST, &condition));

    PROPAGATE_IF_ERROR_DO(parser_expect_peek(p, RPAREN), {
        Node* cond_node = (Node*)condition;
        cond_node->vtable->destroy(cond_node, p->allocator.free_alloc);
    });

    Statement* consequence;
    PROPAGATE_IF_ERROR_DO(_if_expression_parse_branch(p, &consequence), {
        Node* cond_node = (Node*)condition;
        cond_node->vtable->destroy(cond_node, p->allocator.free_alloc);
    });

    Statement* alternate = NULL;
    if (parser_peek_token_is(p, ELSE)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
        PROPAGATE_IF_ERROR_DO(_if_expression_parse_branch(p, &alternate), {
            Node* cond_node = (Node*)condition;
            cond_node->vtable->destroy(cond_node, p->allocator.free_alloc);
            Node* conseq_node = (Node*)consequence;
            conseq_node->vtable->destroy(conseq_node, p->allocator.free_alloc);
        });
    }

    IfExpression* if_expr;
    PROPAGATE_IF_ERROR_DO(
        if_expression_create(
            condition, consequence, alternate, &if_expr, p->allocator.memory_alloc),
        {
            Node* cond_node = (Node*)condition;
            cond_node->vtable->destroy(cond_node, p->allocator.free_alloc);
            Node* conseq_node = (Node*)consequence;
            conseq_node->vtable->destroy(conseq_node, p->allocator.free_alloc);

            if (alternate) {
                Node* alt_node = (Node*)alternate;
                alt_node->vtable->destroy(alt_node, p->allocator.free_alloc);
            }
        });

    *expression = (Expression*)if_expr;
    return SUCCESS;
}

TRY_STATUS function_expression_parse(Parser* p, Expression** expression) {
    PROPAGATE_IF_ERROR(parser_expect_peek(p, LPAREN));

    ArrayList parameters;
    PROPAGATE_IF_ERROR(allocate_parameter_list(p, &parameters, NULL));
    PROPAGATE_IF_ERROR_DO(parser_expect_peek(p, LBRACE), free_parameter_list(&parameters));

    BlockStatement* body;
    PROPAGATE_IF_ERROR_DO(block_statement_parse(p, &body), free_parameter_list(&parameters));

    FunctionExpression* function;
    PROPAGATE_IF_ERROR_DO(
        function_expression_create(parameters, body, &function, p->allocator.memory_alloc), {
            free_parameter_list(&parameters);
            block_statement_destroy((Node*)body, p->allocator.free_alloc);
        });

    *expression = (Expression*)function;
    return SUCCESS;
}
