#include <assert.h>
#include <stdalign.h>

#include "ast/ast.h"
#include "ast/node.h"
#include "ast/statements/block.h"
#include "ast/statements/statement.h"

#include "lexer/keywords.h"
#include "lexer/operators.h"

TRY_STATUS ast_init(AST* ast, Allocator allocator) {
    if (!ast) {
        return NULL_PARAMETER;
    }
    ASSERT_ALLOCATOR(allocator);

    ArrayList statements;
    PROPAGATE_IF_ERROR(array_list_init_allocator(&statements, 64, sizeof(Statement*), allocator));

    HashMap      tt_symbols;
    const size_t num_keywords  = sizeof(ALL_KEYWORDS) / sizeof(ALL_KEYWORDS[0]);
    const size_t num_operators = sizeof(ALL_OPERATORS) / sizeof(ALL_OPERATORS[0]);
    const size_t total_symbols = num_keywords + num_operators;
    PROPAGATE_IF_ERROR_DO(hash_map_init(&tt_symbols,
                                        total_symbols,
                                        sizeof(TokenType),
                                        alignof(TokenType),
                                        sizeof(Slice),
                                        alignof(Slice),
                                        hash_token_type,
                                        compare_token_type),
                          array_list_deinit(&statements));

    for (size_t i = 0; i < num_keywords; i++) {
        const Keyword keyword = ALL_KEYWORDS[i];
        hash_map_put_assume_capacity(&tt_symbols, &keyword.type, &keyword.slice);
    }

    for (size_t i = 0; i < num_operators; i++) {
        const Operator op = ALL_OPERATORS[i];
        hash_map_put_assume_capacity(&tt_symbols, &op.type, &op.slice);
    }

    *ast = (AST){
        .statements         = statements,
        .token_type_symbols = tt_symbols,
        .allocator          = allocator,
    };
    return SUCCESS;
}

void ast_deinit(AST* ast) {
    if (!ast) {
        return;
    }
    ASSERT_ALLOCATOR(ast->allocator);

    clear_statement_list(&ast->statements, ast->allocator.free_alloc);
    array_list_deinit(&ast->statements);
    hash_map_deinit(&ast->token_type_symbols);
}

TRY_STATUS ast_reconstruct(AST* ast, StringBuilder* sb) {
    assert(ast && ast->statements.data);
    assert(sb && sb->buffer.data);

    array_list_clear_retaining_capacity(&sb->buffer);

    for (size_t i = 0; i < ast->statements.length; i++) {
        Statement* stmt;
        UNREACHABLE_IF_ERROR(array_list_get(&ast->statements, i, &stmt));
        ASSERT_STATEMENT(stmt);
        Node* node = (Node*)stmt;
        ASSERT_NODE(node);
        PROPAGATE_IF_ERROR_DO(node->vtable->reconstruct(node, &ast->token_type_symbols, sb),
                              string_builder_deinit(sb));
    }
    return SUCCESS;
}

Slice poll_tt_symbol(const HashMap* symbol_map, TokenType t) {
    assert(symbol_map && symbol_map->buffer);

    Slice slice;
    if (STATUS_OK(hash_map_get_value(symbol_map, &t, &slice))) {
        return slice;
    }

    const char* name = token_type_name(t);
    return slice_from_str_z(name);
}

void clear_statement_list(ArrayList* statements, free_alloc_fn free_alloc) {
    assert(statements && statements->data);
    assert(free_alloc);

    Statement* stmt;
    for (size_t i = 0; i < statements->length; i++) {
        UNREACHABLE_IF_ERROR(array_list_get(statements, i, &stmt));
        ASSERT_STATEMENT(stmt);
        NODE_VIRTUAL_FREE(stmt, free_alloc);
    }

    array_list_clear_retaining_capacity(statements);
}

void clear_expression_list(ArrayList* expressions, free_alloc_fn free_alloc) {
    assert(expressions && expressions->data);
    assert(free_alloc);

    Expression* expr;
    for (size_t i = 0; i < expressions->length; i++) {
        UNREACHABLE_IF_ERROR(array_list_get(expressions, i, &expr));
        ASSERT_EXPRESSION(expr);
        NODE_VIRTUAL_FREE(expr, free_alloc);
    }

    array_list_clear_retaining_capacity(expressions);
}
