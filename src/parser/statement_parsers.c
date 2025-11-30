#include <assert.h>
#include <stdio.h>

#include "lexer/token.h"

#include "ast/expressions/identifier.h"
#include "ast/expressions/type.h"

#include "parser/expression_parsers.h"
#include "parser/statement_parsers.h"

#include "util/allocator.h"

TRY_STATUS decl_statement_parse(Parser* p, DeclStatement** stmt) {
    assert(p);
    ASSERT_ALLOCATOR(p->allocator);
    const Token start_token = p->current_token;

    PROPAGATE_IF_ERROR(parser_expect_peek(p, IDENT));

    IdentifierExpression* ident;
    PROPAGATE_IF_ERROR(identifier_expression_create(
        p->current_token, &ident, p->allocator.memory_alloc, p->allocator.free_alloc));

    Expression* type_expr;
    bool        value_initialized;
    PROPAGATE_IF_ERROR_DO(type_expression_parse(p, &type_expr, &value_initialized),
                          identifier_expression_destroy((Node*)ident, p->allocator.free_alloc));
    TypeExpression* type = (TypeExpression*)type_expr;

    Expression* value = NULL;
    if (value_initialized) {
        PROPAGATE_IF_ERROR_DO(expression_parse(p, LOWEST, &value), {
            identifier_expression_destroy((Node*)ident, p->allocator.free_alloc);
            type_expression_destroy((Node*)type, p->allocator.free_alloc);
        });
    }

    if (parser_peek_token_is(p, SEMICOLON)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
    }

    DeclStatement* decl_stmt;
    const Status   create_status = decl_statement_create(
        start_token, ident, type, value, &decl_stmt, p->allocator.memory_alloc);
    if (STATUS_ERR(create_status)) {
        IGNORE_STATUS(
            parser_put_status_error(p, create_status, start_token.line, start_token.column));

        identifier_expression_destroy((Node*)ident, p->allocator.free_alloc);
        type_expression_destroy((Node*)type, p->allocator.free_alloc);
        NODE_VIRTUAL_FREE(value, p->allocator.free_alloc);
        return create_status;
    }

    *stmt = decl_stmt;
    return SUCCESS;
}

TRY_STATUS type_decl_statement_parse(Parser* p, TypeDeclStatement** stmt) {
    assert(p);
    ASSERT_ALLOCATOR(p->allocator);
    const Token start_token = p->current_token;

    PROPAGATE_IF_ERROR(parser_expect_peek(p, IDENT));

    IdentifierExpression* ident;
    PROPAGATE_IF_ERROR(identifier_expression_create(
        p->current_token, &ident, p->allocator.memory_alloc, p->allocator.free_alloc));

    PROPAGATE_IF_ERROR_DO(parser_expect_peek(p, ASSIGN),
                          identifier_expression_destroy((Node*)ident, p->allocator.free_alloc));

    if (parser_peek_token_is(p, SEMICOLON) || parser_peek_token_is(p, END)) {
        identifier_expression_destroy((Node*)ident, p->allocator.free_alloc);
        return UNEXPECTED_TOKEN;
    }

    Expression* value;
    UNREACHABLE_IF_ERROR(parser_next_token(p));
    bool is_primitive_alias = false;
    if (hash_set_contains(&p->primitives, &p->current_token.type)) {
        is_primitive_alias = true;
        PROPAGATE_IF_ERROR_DO(identifier_expression_parse(p, &value),
                              identifier_expression_destroy((Node*)ident, p->allocator.free_alloc));
    } else {
        PROPAGATE_IF_ERROR_DO(expression_parse(p, LOWEST, &value),
                              identifier_expression_destroy((Node*)ident, p->allocator.free_alloc));
    }

    if (parser_peek_token_is(p, SEMICOLON)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
    }

    TypeDeclStatement* type_decl;
    PROPAGATE_IF_ERROR_DO(
        type_decl_statement_create(
            start_token, ident, value, is_primitive_alias, &type_decl, p->allocator.memory_alloc),
        {
            NODE_VIRTUAL_FREE(value, p->allocator.free_alloc);
            identifier_expression_destroy((Node*)ident, p->allocator.free_alloc);
        });

    *stmt = type_decl;
    return SUCCESS;
}

TRY_STATUS jump_statement_parse(Parser* p, JumpStatement** stmt) {
    assert(p);
    ASSERT_ALLOCATOR(p->allocator);

    const Token start_token = p->current_token;
    UNREACHABLE_IF_ERROR(parser_next_token(p));

    Expression* value = NULL;
    if (!parser_current_token_is(p, SEMICOLON)) {
        PROPAGATE_IF_ERROR(expression_parse(p, LOWEST, &value));
    }

    JumpStatement* jump_stmt;
    PROPAGATE_IF_ERROR_DO(
        jump_statement_create(start_token, value, &jump_stmt, p->allocator.memory_alloc),
        NODE_VIRTUAL_FREE(value, p->allocator.free_alloc));

    if (parser_peek_token_is(p, SEMICOLON)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
    }

    *stmt = jump_stmt;
    return SUCCESS;
}

TRY_STATUS expression_statement_parse(Parser* p, ExpressionStatement** stmt) {
    assert(p);
    ASSERT_ALLOCATOR(p->allocator);
    const Token start_token = p->current_token;

    Expression* lhs;
    PROPAGATE_IF_ERROR(expression_parse(p, LOWEST, &lhs));

    ExpressionStatement* expr_stmt;
    PROPAGATE_IF_ERROR_DO(
        expression_statement_create(start_token, lhs, &expr_stmt, p->allocator.memory_alloc),
        NODE_VIRTUAL_FREE(lhs, p->allocator.free_alloc));

    if (parser_peek_token_is(p, SEMICOLON)) {
        PROPAGATE_IF_ERROR_DO(parser_next_token(p),
                              NODE_VIRTUAL_FREE(lhs, p->allocator.free_alloc));
    }

    *stmt = expr_stmt;
    return SUCCESS;
}

TRY_STATUS block_statement_parse(Parser* p, BlockStatement** stmt) {
    assert(p);
    ASSERT_ALLOCATOR(p->allocator);

    const Token start_token = p->current_token;
    PROPAGATE_IF_ERROR(parser_next_token(p));

    BlockStatement* block;
    PROPAGATE_IF_ERROR(block_statement_create(start_token, &block, p->allocator));

    while (!parser_current_token_is(p, RBRACE) && !parser_current_token_is(p, END)) {
        Statement* inner_stmt;
        PROPAGATE_IF_ERROR_DO(parser_parse_statement(p, &inner_stmt),
                              NODE_VIRTUAL_FREE(block, p->allocator.free_alloc););

        PROPAGATE_IF_ERROR_DO(block_statement_append(block, inner_stmt), {
            NODE_VIRTUAL_FREE(inner_stmt, p->allocator.free_alloc);
            NODE_VIRTUAL_FREE(block, p->allocator.free_alloc);
        });
        UNREACHABLE_IF_ERROR(parser_next_token(p));
    }

    *stmt = block;
    return SUCCESS;
}
