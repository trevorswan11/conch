#include <assert.h>

#include "semantic/context.h"
#include "semantic/symbol.h"
#include "semantic/type.h"

[[nodiscard]] Status
semantic_context_create(SemanticContext* parent, SemanticContext** context, Allocator allocator) {
    ASSERT_ALLOCATOR(allocator);
    SemanticContext* sem_con = allocator.memory_alloc(sizeof(SemanticContext));
    if (!sem_con) { return ALLOCATION_FAILED; }

    SymbolTable* symbols;
    TRY_DO(symbol_table_create(&symbols, allocator), allocator.free_alloc(sem_con));

    *sem_con = (SemanticContext){
        .parent       = parent,
        .symbol_table = symbols,
    };

    *context = sem_con;
    return SUCCESS;
}

Allocator semantic_context_allocator(SemanticContext* context) {
    assert(context && context->symbol_table);
    return context->symbol_table->symbols.allocator;
}

SemanticType* semantic_context_move_analyzed(SemanticContext* context) {
    assert(context);
    SemanticType* type     = context->analyzed_type;
    context->analyzed_type = nullptr;
    return type;
}

void semantic_context_destroy(SemanticContext* context, free_alloc_fn free_alloc) {
    if (!context) { return; }

    symbol_table_destroy(context->symbol_table, free_alloc);
    free_alloc(context);
}

bool semantic_context_find(SemanticContext* context,
                           bool             check_parents,
                           MutSlice         symbol,
                           SemanticType**   type) {
    assert(context);
    if (!check_parents) { return symbol_table_find(context->symbol_table, symbol, type); }

    SemanticContext* current = context;
    while (current != nullptr) {
        if (symbol_table_find(current->symbol_table, symbol, type)) { return true; }

        current = current->parent;
    }

    return false;
}

bool semantic_context_has(SemanticContext* context, bool check_parents, MutSlice symbol) {
    assert(context);
    if (!check_parents) { return symbol_table_has(context->symbol_table, symbol); }

    SemanticContext* current = context;
    while (current != nullptr) {
        if (symbol_table_has(current->symbol_table, symbol)) { return true; }

        current = current->parent;
    }

    return false;
}
