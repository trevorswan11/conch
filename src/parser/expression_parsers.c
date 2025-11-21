#include <assert.h>
#include <stdio.h>

#include "lexer/token.h"

#include "ast/ast.h"
#include "ast/expressions/bool.h"
#include "ast/expressions/expression.h"
#include "ast/expressions/float.h"
#include "ast/expressions/identifier.h"
#include "ast/expressions/infix.h"
#include "ast/expressions/integer.h"
#include "ast/expressions/prefix.h"
#include "ast/expressions/type.h"
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
        const bool is_primitive = hash_set_contains(&p->primitives, &p->peek_token.type);
        if (is_primitive || parser_peek_token_is(p, IDENT)) {
            UNREACHABLE_IF_ERROR(parser_next_token(p));

            Expression* ident_expr;
            PROPAGATE_IF_ERROR(identifier_expression_parse(p, &ident_expr));
            IdentifierExpression* ident = (IdentifierExpression*)ident_expr;

            const TypeUnion onion = (TypeUnion){
                .explicit_type =
                    (ExplicitType){
                        .identifier = ident,
                        .nullable   = is_nullable,
                        .primitive  = is_primitive,
                    },
            };

            PROPAGATE_IF_ERROR_DO(
                type_expression_create(EXPLICIT, onion, &type, p->allocator.memory_alloc), {
                    Node* ident_node = (Node*)ident_expr;
                    ident_node->vtable->destroy(ident_node, p->allocator.free_alloc);
                });
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
    const TokenType current_token_type = p->current_token.type;
    if (current_token_type != TRUE && current_token_type != FALSE) {
        return UNEXPECTED_TOKEN;
    }

    BoolLiteralExpression* boolean;
    PROPAGATE_IF_ERROR(bool_literal_expression_create(
        current_token_type == TRUE, &boolean, p->allocator.memory_alloc));
    *expression = (Expression*)boolean;
    return SUCCESS;
}
