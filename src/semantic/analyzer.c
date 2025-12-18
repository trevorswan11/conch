#include <assert.h>

#include "ast/ast.h"
#include "ast/statements/statement.h"

#include "semantic/analyzer.h"
#include "semantic/context.h"

NODISCARD Status seman_init(const AST* ast, SemanticAnalyzer* analyzer, Allocator allocator) {
    assert(ast);
    TRY(seman_null_init(analyzer, allocator));
    analyzer->ast = ast;
    return SUCCESS;
}

NODISCARD Status seman_null_init(SemanticAnalyzer* analyzer, Allocator allocator) {
    ASSERT_ALLOCATOR(allocator);

    SemanticContext* global_ctx;
    TRY(semantic_context_create(NULL, &global_ctx, allocator));

    ArrayList errors;
    TRY_DO(array_list_init_allocator(&errors, 10, sizeof(MutSlice), allocator),
           semantic_context_destroy(global_ctx, allocator.free_alloc));

    SemanticAnalyzer seman = (SemanticAnalyzer){
        .ast        = NULL,
        .global_ctx = global_ctx,
        .errors     = errors,
        .allocator  = allocator,
    };

    *analyzer = seman;
    return SUCCESS;
}

void seman_deinit(SemanticAnalyzer* analyzer) {
    if (!analyzer) {
        return;
    }
    free_alloc_fn free_alloc = analyzer->allocator.free_alloc;

    semantic_context_destroy(analyzer->global_ctx, free_alloc);
    analyzer->global_ctx = NULL;
    free_error_list(&analyzer->errors, free_alloc);
}

NODISCARD Status seman_analyze(SemanticAnalyzer* analyzer) {
    assert(analyzer);
    assert(analyzer->ast);
    assert(analyzer->global_ctx);

    const Allocator allocator = analyzer->allocator;
    clear_error_list(&analyzer->errors, allocator.free_alloc);

    ArrayListConstIterator it = array_list_const_iterator_init(&analyzer->ast->statements);
    Statement*             stmt;
    while (array_list_const_iterator_has_next(&it, &stmt)) {
        assert(analyzer->global_ctx);
        ASSERT_STATEMENT(stmt);
        TRY_IS(NODE_VIRTUAL_ANALYZE(stmt, analyzer->global_ctx, &analyzer->errors),
               ALLOCATION_FAILED);

        // If the global context never had the last type moved out, we must release it
        if (analyzer->global_ctx->analyzed_type) {
            RC_RELEASE(analyzer->global_ctx->analyzed_type, allocator.free_alloc);
            analyzer->global_ctx->analyzed_type = NULL;
        }
    }

    return SUCCESS;
}
