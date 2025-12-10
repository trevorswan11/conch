#include <assert.h>

#include "lexer/token.h"

#include "parser/expression_parsers.h"
#include "parser/parser.h"
#include "parser/statement_parsers.h"

#include "ast/expressions/identifier.h"
#include "ast/expressions/string.h"
#include "ast/expressions/type.h"
#include "ast/node.h"
#include "ast/statements/block.h"
#include "ast/statements/declarations.h"
#include "ast/statements/discard.h"
#include "ast/statements/expression.h"
#include "ast/statements/impl.h"
#include "ast/statements/import.h"
#include "ast/statements/jump.h"

#include "parser/precedence.h"
#include "util/allocator.h"
#include "util/status.h"

NODISCARD Status decl_statement_parse(Parser* p, DeclStatement** stmt) {
    assert(p);
    ASSERT_ALLOCATOR(p->allocator);

    const Token start_token = p->current_token;
    TRY(parser_expect_peek(p, IDENT));

    Expression* ident_expr;
    TRY(identifier_expression_parse(p, &ident_expr));
    IdentifierExpression* ident = (IdentifierExpression*)ident_expr;

    Expression* type_expr;
    bool        value_initialized;
    TRY_DO(type_expression_parse(p, &type_expr, &value_initialized),
           identifier_expression_destroy((Node*)ident, p->allocator.free_alloc));
    TypeExpression* type = (TypeExpression*)type_expr;

    Expression* value = NULL;
    if (value_initialized) {
        TRY_DO(expression_parse(p, LOWEST, &value), {
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

NODISCARD Status type_decl_statement_parse(Parser* p, TypeDeclStatement** stmt) {
    assert(p);
    ASSERT_ALLOCATOR(p->allocator);

    const Token start_token = p->current_token;
    TRY(parser_expect_peek(p, IDENT));

    Expression* ident_expr;
    TRY(identifier_expression_parse(p, &ident_expr));
    IdentifierExpression* ident = (IdentifierExpression*)ident_expr;

    TRY_DO(parser_expect_peek(p, ASSIGN),
           identifier_expression_destroy((Node*)ident, p->allocator.free_alloc));

    if (parser_peek_token_is(p, SEMICOLON) || parser_peek_token_is(p, END)) {
        identifier_expression_destroy((Node*)ident, p->allocator.free_alloc);
        return UNEXPECTED_TOKEN;
    }

    Expression* value;
    bool        is_primitive_alias = false;
    if (hash_set_contains(&p->primitives, &p->current_token.type)) {
        is_primitive_alias = true;
        UNREACHABLE_IF_ERROR(parser_next_token(p));
        TRY_DO(identifier_expression_parse(p, &value),
               identifier_expression_destroy((Node*)ident, p->allocator.free_alloc));
    } else {
        TypeExpression* type;
        TRY_DO(explicit_type_parse(p, p->current_token, &type),
               identifier_expression_destroy((Node*)ident, p->allocator.free_alloc));
        value = (Expression*)type;
    }

    if (parser_peek_token_is(p, SEMICOLON)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
    }

    TypeDeclStatement* type_decl;
    TRY_DO(
        type_decl_statement_create(
            start_token, ident, value, is_primitive_alias, &type_decl, p->allocator.memory_alloc),
        {
            NODE_VIRTUAL_FREE(value, p->allocator.free_alloc);
            identifier_expression_destroy((Node*)ident, p->allocator.free_alloc);
        });

    *stmt = type_decl;
    return SUCCESS;
}

NODISCARD Status jump_statement_parse(Parser* p, JumpStatement** stmt) {
    assert(p);
    ASSERT_ALLOCATOR(p->allocator);

    const Token start_token = p->current_token;
    Expression* value       = NULL;

    if (start_token.type != CONTINUE && !parser_peek_token_is(p, END)) {
        TRY(parser_next_token(p));

        if (!parser_current_token_is(p, SEMICOLON)) {
            TRY(expression_parse(p, LOWEST, &value));
        }
    }

    JumpStatement* jump_stmt;
    TRY_DO(jump_statement_create(start_token, value, &jump_stmt, p->allocator.memory_alloc),
           NODE_VIRTUAL_FREE(value, p->allocator.free_alloc));

    if (parser_peek_token_is(p, SEMICOLON)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
    }

    *stmt = jump_stmt;
    return SUCCESS;
}

NODISCARD Status expression_statement_parse(Parser* p, ExpressionStatement** stmt) {
    assert(p);
    ASSERT_ALLOCATOR(p->allocator);
    const Token start_token = p->current_token;

    Expression* lhs;
    TRY(expression_parse(p, LOWEST, &lhs));

    ExpressionStatement* expr_stmt;
    TRY_DO(expression_statement_create(start_token, lhs, &expr_stmt, p->allocator.memory_alloc),
           NODE_VIRTUAL_FREE(lhs, p->allocator.free_alloc));

    if (parser_peek_token_is(p, SEMICOLON)) {
        TRY_DO(parser_next_token(p), NODE_VIRTUAL_FREE(lhs, p->allocator.free_alloc));
    }

    *stmt = expr_stmt;
    return SUCCESS;
}

NODISCARD Status block_statement_parse(Parser* p, BlockStatement** stmt) {
    assert(p);
    ASSERT_ALLOCATOR(p->allocator);

    const Token     start_token = p->current_token;
    BlockStatement* block;
    TRY(block_statement_create(start_token, &block, p->allocator));

    while (!parser_peek_token_is(p, RBRACE) && !parser_peek_token_is(p, END)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));

        Statement* inner_stmt;
        TRY_DO(parser_parse_statement(p, &inner_stmt),
               NODE_VIRTUAL_FREE(block, p->allocator.free_alloc););

        TRY_DO(block_statement_append(block, inner_stmt), {
            NODE_VIRTUAL_FREE(inner_stmt, p->allocator.free_alloc);
            NODE_VIRTUAL_FREE(block, p->allocator.free_alloc);
        });
    }

    TRY_DO(parser_expect_peek(p, RBRACE),
           block_statement_destroy((Node*)block, p->allocator.free_alloc));

    *stmt = block;
    return SUCCESS;
}

NODISCARD Status impl_statement_parse(Parser* p, ImplStatement** stmt) {
    assert(p);
    ASSERT_ALLOCATOR(p->allocator);

    const Token start_token = p->current_token;
    TRY(parser_expect_peek(p, IDENT));

    Expression* ident_expr;
    TRY(identifier_expression_parse(p, &ident_expr));
    IdentifierExpression* ident = (IdentifierExpression*)ident_expr;
    TRY_DO(parser_expect_peek(p, LBRACE),
           identifier_expression_destroy((Node*)ident, p->allocator.free_alloc));

    BlockStatement* block;
    TRY_DO(block_statement_parse(p, &block),
           identifier_expression_destroy((Node*)ident, p->allocator.free_alloc));

    if (block->statements.length == 0) {
        IGNORE_STATUS(
            parser_put_status_error(p, EMPTY_IMPL_BLOCK, start_token.line, start_token.column));

        identifier_expression_destroy((Node*)ident, p->allocator.free_alloc);
        block_statement_destroy((Node*)block, p->allocator.free_alloc);
        return EMPTY_IMPL_BLOCK;
    }

    ImplStatement* impl;
    TRY_DO(impl_statement_create(start_token, ident, block, &impl, p->allocator.memory_alloc), {
        identifier_expression_destroy((Node*)ident, p->allocator.free_alloc);
        block_statement_destroy((Node*)block, p->allocator.free_alloc);
    });

    if (parser_peek_token_is(p, SEMICOLON)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
    }

    *stmt = impl;
    return SUCCESS;
}

NODISCARD Status import_statement_parse(Parser* p, ImportStatement** stmt) {
    assert(p);
    ASSERT_ALLOCATOR(p->allocator);

    const Token start_token = p->current_token;
    ImportTag   tag;
    ImportUnion variant;
    Expression* payload;

    switch (p->peek_token.type) {
    case IDENT: {
        UNREACHABLE_IF_ERROR(parser_expect_peek(p, IDENT));
        TRY(identifier_expression_parse(p, &payload));
        IdentifierExpression* ident = (IdentifierExpression*)payload;

        tag     = STANDARD;
        variant = (ImportUnion){.standard_import = ident};
        break;
    }
    case STRING: {
        UNREACHABLE_IF_ERROR(parser_expect_peek(p, STRING));
        TRY(string_expression_parse(p, &payload));
        StringLiteralExpression* string = (StringLiteralExpression*)payload;

        tag     = USER;
        variant = (ImportUnion){.user_import = string};
        break;
    }
    default:
        IGNORE_STATUS(
            parser_put_status_error(p, UNEXPECTED_TOKEN, p->peek_token.line, p->peek_token.column));
        return UNEXPECTED_TOKEN;
    }

    IdentifierExpression* alias = NULL;
    if (parser_peek_token_is(p, AS)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
        TRY_DO(parser_expect_peek(p, IDENT), NODE_VIRTUAL_FREE(payload, p->allocator.free_alloc));

        Expression* ident;
        TRY_DO(identifier_expression_parse(p, &ident),
               NODE_VIRTUAL_FREE(payload, p->allocator.free_alloc));
        alias = (IdentifierExpression*)ident;
    } else if (tag == USER) {
        IGNORE_STATUS(parser_put_status_error(
            p, USER_IMPORT_MISSING_ALIAS, start_token.line, start_token.column));
        NODE_VIRTUAL_FREE(payload, p->allocator.free_alloc);
        return USER_IMPORT_MISSING_ALIAS;
    }

    ImportStatement* import;
    TRY_DO(import_statement_create(
               start_token, tag, variant, alias, &import, p->allocator.memory_alloc),
           NODE_VIRTUAL_FREE(payload, p->allocator.free_alloc));

    if (parser_peek_token_is(p, SEMICOLON)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
    }

    *stmt = import;
    return SUCCESS;
}

NODISCARD Status discard_statement_parse(Parser* p, DiscardStatement** stmt) {
    assert(p);
    ASSERT_ALLOCATOR(p->allocator);

    const Token start_token = p->current_token;
    TRY(parser_expect_peek(p, ASSIGN));
    TRY(parser_next_token(p));

    Expression* to_discard;
    TRY(expression_parse(p, LOWEST, &to_discard));

    DiscardStatement* discard;
    TRY_DO(discard_statement_create(start_token, to_discard, &discard, p->allocator.memory_alloc),
           NODE_VIRTUAL_FREE(to_discard, p->allocator.free_alloc));

    if (parser_peek_token_is(p, SEMICOLON)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
    }

    *stmt = discard;
    return SUCCESS;
}
