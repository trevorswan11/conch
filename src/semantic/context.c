#include <assert.h>
#include <stdalign.h>

#include "semantic/context.h"
#include "semantic/symbol.h"

NODISCARD Status semantic_context_create(SemanticContext*  parent,
                                         SemanticContext** context,
                                         Allocator         allocator) {
    ASSERT_ALLOCATOR(allocator);
    SemanticContext* sem_con = allocator.memory_alloc(sizeof(SemanticContext));
    if (!sem_con) {
        return ALLOCATION_FAILED;
    }

    SymbolTable* symbols;
    TRY_DO(symbol_table_create(&symbols, allocator), allocator.free_alloc(sem_con));

    *sem_con = (SemanticContext){
        .parent       = parent,
        .symbol_table = symbols,
    };

    *context = sem_con;
    return SUCCESS;
}

void semantic_context_destroy(SemanticContext* context, free_alloc_fn free_alloc) {
    if (!context) {
        return;
    }

    symbol_table_destroy(context->symbol_table, free_alloc);
    free_alloc(context);
}
