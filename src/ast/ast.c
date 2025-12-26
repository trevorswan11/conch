#include <assert.h>
#include <stdalign.h>

#include "ast/ast.h"
#include "ast/expressions/expression.h"
#include "ast/node.h"
#include "ast/statements/statement.h"

#include "lexer/keywords.h"
#include "lexer/operators.h"

#include "util/containers/string_builder.h"

bool group_expressions = false;

void clear_error_list(ArrayList* errors, free_alloc_fn free_alloc) {
    if (!errors) { return; }
    assert(free_alloc);

    ArrayListConstIterator it = array_list_const_iterator_init(errors);
    MutSlice               error;
    while (array_list_const_iterator_has_next(&it, &error)) {
        free_alloc(error.ptr);
    }

    array_list_clear_retaining_capacity(errors);
}

void free_error_list(ArrayList* errors, free_alloc_fn free_alloc) {
    if (!errors || !errors->data) { return; }
    assert(free_alloc);

    clear_error_list(errors, free_alloc);
    array_list_deinit(errors);
}

NODISCARD Status ast_init(AST* ast, Allocator allocator) {
    assert(ast);
    ASSERT_ALLOCATOR(allocator);

    ArrayList statements;
    TRY(array_list_init_allocator(&statements, 64, sizeof(Statement*), allocator));

    HashMap      tt_symbols;
    const size_t num_keywords  = sizeof(ALL_KEYWORDS) / sizeof(ALL_KEYWORDS[0]);
    const size_t num_operators = sizeof(ALL_OPERATORS) / sizeof(ALL_OPERATORS[0]);
    const size_t total_symbols = num_keywords + num_operators;
    TRY_DO(hash_map_init_allocator(&tt_symbols,
                                   total_symbols,
                                   sizeof(TokenType),
                                   alignof(TokenType),
                                   sizeof(Slice),
                                   alignof(Slice),
                                   hash_token_type,
                                   compare_token_type,
                                   allocator),
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
    if (!ast) { return; }
    ASSERT_ALLOCATOR(ast->allocator);

    free_statement_list(&ast->statements, ast->allocator.free_alloc);
    hash_map_deinit(&ast->token_type_symbols);
}

NODISCARD Status ast_reconstruct(AST* ast, StringBuilder* sb) {
    assert(ast && ast->statements.data);
    assert(sb && sb->buffer.data);

    array_list_clear_retaining_capacity(&sb->buffer);

    ArrayListConstIterator it = array_list_const_iterator_init(&ast->statements);
    Statement*             stmt;
    while (array_list_const_iterator_has_next(&it, (void*)&stmt)) {
        ASSERT_STATEMENT(stmt);
        TRY_DO(NODE_VIRTUAL_RECONSTRUCT(stmt, &ast->token_type_symbols, sb),
               string_builder_deinit(sb));
    }
    return SUCCESS;
}

Slice poll_tt_symbol(const HashMap* symbol_map, TokenType t) {
    assert(symbol_map && symbol_map->buffer);

    Slice slice;
    if (STATUS_OK(hash_map_get_value(symbol_map, &t, &slice))) { return slice; }

    const char* name = token_type_name(t);
    return slice_from_str_z(name);
}

void clear_statement_list(ArrayList* statements, free_alloc_fn free_alloc) {
    assert(statements && statements->data);
    assert(free_alloc);

    ArrayListConstIterator it = array_list_const_iterator_init(statements);
    Statement*             stmt;
    while (array_list_const_iterator_has_next(&it, (void*)&stmt)) {
        ASSERT_STATEMENT(stmt);
        NODE_VIRTUAL_FREE(stmt, free_alloc);
    }

    array_list_clear_retaining_capacity(statements);
}

void clear_expression_list(ArrayList* expressions, free_alloc_fn free_alloc) {
    assert(expressions && expressions->data);
    assert(free_alloc);

    ArrayListConstIterator it = array_list_const_iterator_init(expressions);
    Expression*            expr;
    while (array_list_const_iterator_has_next(&it, (void*)&expr)) {
        ASSERT_EXPRESSION(expr);
        NODE_VIRTUAL_FREE(expr, free_alloc);
    }

    array_list_clear_retaining_capacity(expressions);
}

void free_statement_list(ArrayList* statements, free_alloc_fn free_alloc) {
    if (!statements || !statements->data) { return; }
    assert(free_alloc);

    clear_statement_list(statements, free_alloc);
    array_list_deinit(statements);
}

void free_expression_list(ArrayList* expressions, free_alloc_fn free_alloc) {
    if (!expressions || !expressions->data) { return; }
    assert(free_alloc);

    clear_expression_list(expressions, free_alloc);
    array_list_deinit(expressions);
}

NODISCARD Status generics_reconstruct(ArrayList*     generics,
                                      const HashMap* symbol_map,
                                      StringBuilder* sb) {
    assert(generics && symbol_map && sb);

    if (generics->length > 0) {
        assert(generics->data);
        TRY(string_builder_append(sb, '<'));

        ArrayListConstIterator it = array_list_const_iterator_init(generics);
        Expression*            generic;
        while (array_list_const_iterator_has_next(&it, (void*)&generic)) {
            TRY(NODE_VIRTUAL_RECONSTRUCT(generic, symbol_map, sb));
            if (!array_list_const_iterator_exhausted(&it)) {
                TRY(string_builder_append_str_z(sb, ", "));
            }
        }

        TRY(string_builder_append(sb, '>'));
    }

    return SUCCESS;
}
