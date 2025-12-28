#include <assert.h>

#include "lexer/token.h"

#include "parser/expression_parsers.h"
#include "parser/statement_parsers.h"

#include "ast/expressions/identifier.h"
#include "ast/expressions/type.h"
#include "ast/statements/block.h"
#include "ast/statements/declarations.h"
#include "ast/statements/discard.h"
#include "ast/statements/expression.h"
#include "ast/statements/impl.h"
#include "ast/statements/import.h"
#include "ast/statements/jump.h"

#include "util/status.h"

[[nodiscard]] Status decl_statement_parse(Parser* p, DeclStatement** stmt) {
    Allocator*  allocator   = parser_allocator(p);
    const Token start_token = p->current_token;
    TRY(parser_expect_peek(p, IDENT));

    Expression* ident_expr;
    TRY(identifier_expression_parse(p, &ident_expr));
    IdentifierExpression* ident = (IdentifierExpression*)ident_expr;

    Expression* type_expr;
    bool        value_initialized;
    TRY_DO(type_expression_parse(p, &type_expr, &value_initialized),
           NODE_VIRTUAL_FREE(ident, allocator));
    TypeExpression* type = (TypeExpression*)type_expr;

    Expression* value = nullptr;
    if (value_initialized) {
        TRY_DO(expression_parse(p, LOWEST, &value), {
            NODE_VIRTUAL_FREE(ident, allocator);
            NODE_VIRTUAL_FREE(type, allocator);
        });
    }

    DeclStatement* decl_stmt;
    const Status   create_status =
        decl_statement_create(start_token, ident, type, value, &decl_stmt, allocator);
    if (STATUS_ERR(create_status)) {
        PUT_STATUS_PROPAGATE(&p->errors, create_status, start_token, {
            NODE_VIRTUAL_FREE(ident, allocator);
            NODE_VIRTUAL_FREE(type, allocator);
            NODE_VIRTUAL_FREE(value, allocator);
        });
    }

    *stmt = decl_stmt;
    return SUCCESS;
}

[[nodiscard]] Status type_decl_statement_parse(Parser* p, TypeDeclStatement** stmt) {
    Allocator*  allocator   = parser_allocator(p);
    const Token start_token = p->current_token;
    TRY(parser_expect_peek(p, IDENT));

    Expression* ident_expr;
    TRY(identifier_expression_parse(p, &ident_expr));
    IdentifierExpression* ident = (IdentifierExpression*)ident_expr;

    TRY_DO(parser_expect_peek(p, ASSIGN), NODE_VIRTUAL_FREE(ident, allocator));

    if (parser_peek_token_is(p, SEMICOLON) || parser_peek_token_is(p, END)) {
        NODE_VIRTUAL_FREE(ident, allocator);
        return UNEXPECTED_TOKEN;
    }

    Expression* value;
    bool        is_primitive_alias = false;
    if (hash_set_contains(&p->primitives, &p->peek_token.type)) {
        is_primitive_alias = true;
        UNREACHABLE_IF_ERROR(parser_next_token(p));
        TRY_DO(identifier_expression_parse(p, &value), NODE_VIRTUAL_FREE(ident, allocator));
    } else {
        TypeExpression* type;
        if (STATUS_ERR(explicit_type_parse(p, p->current_token, &type))) {
            PUT_STATUS_PROPAGATE(
                &p->errors, MALFORMED_TYPE_DECL, start_token, NODE_VIRTUAL_FREE(ident, allocator));
        }
        value = (Expression*)type;
    }

    TypeDeclStatement* type_decl;
    TRY_DO(type_decl_statement_create(
               start_token, ident, value, is_primitive_alias, &type_decl, allocator),
           {
               NODE_VIRTUAL_FREE(value, allocator);
               NODE_VIRTUAL_FREE(ident, allocator);
           });

    *stmt = type_decl;
    return SUCCESS;
}

[[nodiscard]] Status jump_statement_parse(Parser* p, JumpStatement** stmt) {
    Allocator*  allocator   = parser_allocator(p);
    const Token start_token = p->current_token;
    Expression* value       = nullptr;

    if (start_token.type != CONTINUE && !parser_peek_token_is(p, END) &&
        !parser_peek_token_is(p, SEMICOLON)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
        TRY(expression_parse(p, LOWEST, &value));
    }

    JumpStatement* jump_stmt;
    TRY_DO(jump_statement_create(start_token, value, &jump_stmt, allocator),
           NODE_VIRTUAL_FREE(value, allocator));

    *stmt = jump_stmt;
    return SUCCESS;
}

[[nodiscard]] Status expression_statement_parse(Parser* p, ExpressionStatement** stmt) {
    Allocator*  allocator   = parser_allocator(p);
    const Token start_token = p->current_token;

    Expression* lhs;
    TRY(expression_parse(p, LOWEST, &lhs));

    ExpressionStatement* expr_stmt;
    TRY_DO(expression_statement_create(start_token, lhs, &expr_stmt, allocator),
           NODE_VIRTUAL_FREE(lhs, allocator));

    *stmt = expr_stmt;
    return SUCCESS;
}

[[nodiscard]] Status block_statement_parse(Parser* p, BlockStatement** stmt) {
    Allocator*  allocator   = parser_allocator(p);
    const Token start_token = p->current_token;

    BlockStatement* block;
    TRY(block_statement_create(start_token, &block, allocator));

    while (!parser_peek_token_is(p, RBRACE) && !parser_peek_token_is(p, END)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));

        Statement* inner_stmt;
        TRY_DO(parser_parse_statement(p, &inner_stmt), NODE_VIRTUAL_FREE(block, allocator););

        TRY_DO(block_statement_append(block, inner_stmt), {
            NODE_VIRTUAL_FREE(inner_stmt, allocator);
            NODE_VIRTUAL_FREE(block, allocator);
        });
    }

    TRY_DO(parser_expect_peek(p, RBRACE), NODE_VIRTUAL_FREE(block, allocator));

    *stmt = block;
    return SUCCESS;
}

[[nodiscard]] Status impl_statement_parse(Parser* p, ImplStatement** stmt) {
    Allocator*  allocator   = parser_allocator(p);
    const Token start_token = p->current_token;

    TRY(parser_expect_peek(p, IDENT));

    Expression* ident_expr;
    TRY(identifier_expression_parse(p, &ident_expr));
    IdentifierExpression* ident = (IdentifierExpression*)ident_expr;
    TRY_DO(parser_expect_peek(p, LBRACE), NODE_VIRTUAL_FREE(ident, allocator));

    BlockStatement* block;
    TRY_DO(block_statement_parse(p, &block), NODE_VIRTUAL_FREE(ident, allocator));

    if (block->statements.length == 0) {
        PUT_STATUS_PROPAGATE(&p->errors, EMPTY_IMPL_BLOCK, start_token, {
            NODE_VIRTUAL_FREE(ident, allocator);
            NODE_VIRTUAL_FREE(block, allocator);
        });
    }

    ImplStatement* impl;
    TRY_DO(impl_statement_create(start_token, ident, block, &impl, allocator), {
        NODE_VIRTUAL_FREE(ident, allocator);
        NODE_VIRTUAL_FREE(block, allocator);
    });

    *stmt = impl;
    return SUCCESS;
}

[[nodiscard]] Status import_statement_parse(Parser* p, ImportStatement** stmt) {
    Allocator*  allocator   = parser_allocator(p);
    const Token start_token = p->current_token;

    ImportTag   tag;
    ImportUnion variant;
    Expression* payload;

    switch (p->peek_token.type) {
    case IDENT:
        UNREACHABLE_IF_ERROR(parser_expect_peek(p, IDENT));
        TRY(identifier_expression_parse(p, &payload));
        IdentifierExpression* ident = (IdentifierExpression*)payload;

        tag     = STANDARD;
        variant = (ImportUnion){.standard_import = ident};
        break;
    case STRING:
        UNREACHABLE_IF_ERROR(parser_expect_peek(p, STRING));
        TRY(string_expression_parse(p, &payload));
        StringLiteralExpression* string = (StringLiteralExpression*)payload;

        tag     = USER;
        variant = (ImportUnion){.user_import = string};
        break;
    default:
        PUT_STATUS_PROPAGATE(&p->errors, UNEXPECTED_TOKEN, p->peek_token, {});
    }

    IdentifierExpression* alias = nullptr;
    if (parser_peek_token_is(p, AS)) {
        UNREACHABLE_IF_ERROR(parser_next_token(p));
        TRY_DO(parser_expect_peek(p, IDENT), NODE_VIRTUAL_FREE(payload, allocator));

        Expression* ident;
        TRY_DO(identifier_expression_parse(p, &ident), NODE_VIRTUAL_FREE(payload, allocator));
        alias = (IdentifierExpression*)ident;
    } else if (tag == USER) {
        PUT_STATUS_PROPAGATE(&p->errors,
                             USER_IMPORT_MISSING_ALIAS,
                             start_token,
                             NODE_VIRTUAL_FREE(payload, allocator));
    }

    ImportStatement* import;
    TRY_DO(import_statement_create(start_token, tag, variant, alias, &import, allocator),
           NODE_VIRTUAL_FREE(payload, allocator));

    *stmt = import;
    return SUCCESS;
}

[[nodiscard]] Status discard_statement_parse(Parser* p, DiscardStatement** stmt) {
    Allocator*  allocator   = parser_allocator(p);
    const Token start_token = p->current_token;

    TRY(parser_expect_peek(p, ASSIGN));
    TRY(parser_next_token(p));

    Expression* to_discard;
    TRY(expression_parse(p, LOWEST, &to_discard));

    DiscardStatement* discard;
    TRY_DO(discard_statement_create(start_token, to_discard, &discard, allocator),
           NODE_VIRTUAL_FREE(to_discard, allocator));

    *stmt = discard;
    return SUCCESS;
}
