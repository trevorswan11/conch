#include <_stdio.h>
#include <assert.h>

#include "ast/ast.h"

#include "ast/statements/statement.h"
#include "semantic/analyzer.h"
#include "semantic/context.h"
#include "util/status.h"

NODISCARD Status seman_init(AST* ast, SemanticAnalyzer* analyzer, Allocator allocator) {
    assert(ast);
    ASSERT_ALLOCATOR(allocator);

    SemanticContext* global_ctx;
    TRY(semantic_context_create(NULL, &global_ctx, allocator));

    ArrayList errors;
    TRY_DO(array_list_init_allocator(&errors, 10, sizeof(MutSlice), allocator),
           semantic_context_destroy(global_ctx, allocator.free_alloc));

    SemanticAnalyzer seman = (SemanticAnalyzer){
        .ast        = ast,
        .global_ctx = global_ctx,
        .errors     = errors,
    };

    *analyzer = seman;
    return SUCCESS;
}

void seman_deinit(SemanticAnalyzer* analyzer, free_alloc_fn free_alloc) {
    if (!analyzer) {
        return;
    }

    ast_deinit(analyzer->ast);
    analyzer->ast = NULL;
    semantic_context_destroy(analyzer->global_ctx, free_alloc);
    analyzer->global_ctx = NULL;
    clear_error_list(&analyzer->errors, free_alloc);
}

NODISCARD Status seman_analyze(SemanticAnalyzer* analyzer) {
    assert(analyzer);

    Statement*       statement;
    const ArrayList* statements = &analyzer->ast->statements;
    for (size_t i = 0; i < statements->length; i++) {
        UNREACHABLE_IF_ERROR(array_list_get(statements, i, &statement));
        Node* node = (Node*)statement;
        TRY_IS(node->vtable->analyze(node, analyzer->global_ctx, &analyzer->errors),
               ALLOCATION_FAILED);
    }

    return SUCCESS;
}
