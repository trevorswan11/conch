#include <assert.h>

#include "semantic/context.h"
#include "semantic/symbol.h"
#include "semantic/type.h"

[[nodiscard]] Status
semantic_context_create(SemanticContext* parent, SemanticContext** context, Allocator* allocator) {
    ASSERT_ALLOCATOR_PTR(allocator);
    SemanticContext* sema_con = ALLOCATOR_PTR_MALLOC(allocator, sizeof(*sema_con));
    if (!sema_con) { return ALLOCATION_FAILED; }

    SymbolTable* symbols;
    TRY_DO(symbol_table_create(&symbols, allocator), ALLOCATOR_PTR_FREE(allocator, sema_con));

    *sema_con = (SemanticContext){
        .parent       = parent,
        .symbol_table = symbols,
    };

    *context = sema_con;
    return SUCCESS;
}

Allocator* semantic_context_allocator(SemanticContext* context) {
    assert(context && context->symbol_table);
    ASSERT_ALLOCATOR(context->symbol_table->symbols.allocator);
    return &context->symbol_table->symbols.allocator;
}

SemanticType* semantic_context_move_analyzed(SemanticContext* context) {
    assert(context);
    SemanticType* type     = context->analyzed_type;
    context->analyzed_type = nullptr;
    return type;
}

void semantic_context_destroy(SemanticContext* context, Allocator* allocator) {
    if (!context) { return; }
    ASSERT_ALLOCATOR_PTR(allocator);

    symbol_table_destroy(context->symbol_table, allocator);
    ALLOCATOR_PTR_FREE(allocator, context);
}

bool semantic_context_find(const SemanticContext* context,
                           bool                   check_parents,
                           MutSlice               symbol,
                           SemanticType**         type) {
    assert(context);
    if (!check_parents) { return symbol_table_find(context->symbol_table, symbol, type); }

    const SemanticContext* current = context;
    while (current != nullptr) {
        if (symbol_table_find(current->symbol_table, symbol, type)) { return true; }
        current = current->parent;
    }

    return false;
}

bool semantic_context_has(const SemanticContext* context, bool check_parents, MutSlice symbol) {
    assert(context);
    if (!check_parents) { return symbol_table_has(context->symbol_table, symbol); }

    const SemanticContext* current = context;
    while (current != nullptr) {
        if (symbol_table_has(current->symbol_table, symbol)) { return true; }

        current = current->parent;
    }

    return false;
}
