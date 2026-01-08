#include <assert.h>

#include "ast/ast.h"
#include "ast/expressions/expression.h"
#include "ast/expressions/identifier.h"
#include "ast/statements/statement.h"

#include "semantic/analyzer.h"
#include "semantic/context.h"

[[nodiscard]] Status generics_analyze(const ArrayList*       generic_exprs,
                                      const SemanticContext* context,
                                      ArrayList*             generics,
                                      Allocator*             allocator) {
    assert(generic_exprs);
    ASSERT_ALLOCATOR_PTR(allocator);

    ArrayList analyzed_generics;
    TRY(array_list_init_allocator(
        &analyzed_generics, generic_exprs->length, sizeof(MutSlice), allocator));
    if (analyzed_generics.length == 0) { return SUCCESS; }

    ArrayListConstIterator it = array_list_const_iterator_init(generic_exprs);
    Expression*            generic_expr;
    while (array_list_const_iterator_has_next(&it, (void*)&generic_expr)) {
        IdentifierExpression* ident = (IdentifierExpression*)generic_expr;
        MutSlice              copied_slice;
        TRY_DO(mut_slice_dupe(&copied_slice, &ident->name, allocator),
               analyzed_generics_deinit(&analyzed_generics, allocator));

        // TODO: switch functions to const pointers
        semantic_context_has(context, true, copied_slice);
        array_list_push_assume_capacity(&analyzed_generics, &copied_slice);
    }

    *generics = analyzed_generics;
    return SUCCESS;
}

void analyzed_generics_deinit(ArrayList* generics, Allocator* allocator) {
    if (!generics) { return; }
    ASSERT_ALLOCATOR_PTR(allocator);

    ArrayListConstIterator it = array_list_const_iterator_init(generics);
    MutSlice               name;
    while (array_list_const_iterator_has_next(&it, (void*)&name)) {
        ALLOCATOR_PTR_FREE(allocator, name.ptr);
    }

    array_list_deinit(generics);
}

[[nodiscard]] Status seman_init(const AST* ast, SemanticAnalyzer* analyzer, Allocator* allocator) {
    assert(ast);
    TRY(seman_null_init(analyzer, allocator));
    analyzer->ast = ast;
    return SUCCESS;
}

[[nodiscard]] Status seman_null_init(SemanticAnalyzer* analyzer, Allocator* allocator) {
    ASSERT_ALLOCATOR_PTR(allocator);

    SemanticContext* global_ctx;
    TRY(semantic_context_create(nullptr, &global_ctx, allocator));

    ArrayList errors;
    TRY_DO(array_list_init_allocator(&errors, 10, sizeof(MutSlice), allocator),
           semantic_context_destroy(global_ctx, allocator));

    SemanticAnalyzer seman = (SemanticAnalyzer){
        .ast        = nullptr,
        .global_ctx = global_ctx,
        .errors     = errors,
        .allocator  = *allocator,
    };

    *analyzer = seman;
    return SUCCESS;
}

void seman_deinit(SemanticAnalyzer* analyzer) {
    if (!analyzer) { return; }
    Allocator* allocator = &analyzer->allocator;
    ASSERT_ALLOCATOR_PTR(allocator);

    semantic_context_destroy(analyzer->global_ctx, allocator);
    analyzer->global_ctx = nullptr;
    free_error_list(&analyzer->errors, allocator);
}

[[nodiscard]] Status seman_analyze(SemanticAnalyzer* analyzer) {
    assert(analyzer);
    assert(analyzer->ast);
    assert(analyzer->global_ctx);

    Allocator* allocator = &analyzer->allocator;
    ASSERT_ALLOCATOR_PTR(allocator);
    clear_error_list(&analyzer->errors, allocator);

    ArrayListConstIterator it = array_list_const_iterator_init(&analyzer->ast->statements);
    Statement*             stmt;
    while (array_list_const_iterator_has_next(&it, (void*)&stmt)) {
        assert(analyzer->global_ctx);
        ASSERT_STATEMENT(stmt);
        TRY_IS(NODE_VIRTUAL_ANALYZE(stmt, analyzer->global_ctx, &analyzer->errors),
               ALLOCATION_FAILED);

        // If the global context never had the last type moved out, we must release it
        if (analyzer->global_ctx->analyzed_type) {
            RC_RELEASE(analyzer->global_ctx->analyzed_type, allocator);
            analyzer->global_ctx->analyzed_type = nullptr;
        }
    }

    return SUCCESS;
}
