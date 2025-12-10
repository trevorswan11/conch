#include <assert.h>
#include <stdalign.h>

#include "semantic/context.h"
#include "semantic/symbol.h"
#include "util/containers/hash_map.h"
#include "util/hash.h"
#include "util/mem.h"

NODISCARD Status semantic_context_create(SemanticContext*  parent,
                                         SemanticContext** context,
                                         Allocator         allocator) {
    ASSERT_ALLOCATOR(allocator);
    SemanticContext* sem_con = allocator.memory_alloc(sizeof(SemanticContext));
    if (!sem_con) {
        return ALLOCATION_FAILED;
    }

    HashMap symbols;
    TRY_DO(hash_map_init_allocator(&symbols,
                                   8,
                                   sizeof(Slice),
                                   alignof(Slice),
                                   sizeof(SemanticSymbol),
                                   alignof(SemanticSymbol),
                                   hash_slice,
                                   compare_slices,
                                   allocator),
           allocator.free_alloc(sem_con));

    *sem_con = (SemanticContext){
        .parent       = parent,
        .symbol_table = symbols,
        .allocator    = allocator,
    };

    *context = sem_con;
    return SUCCESS;
}

void semantic_context_destroy(SemanticContext* context) {
    if (!context) {
        return;
    }
    ASSERT_ALLOCATOR(context->allocator);
    free_alloc_fn free_alloc = context->allocator.free_alloc;

    hash_map_deinit(&context->symbol_table);
    free_alloc(context);
}

void semantic_context_clear_lineage(SemanticContext* context) {
    if (!context) {
        return;
    }

    do {
        SemanticContext* next = context->parent;
        semantic_context_destroy(context);
        context = next;
    } while (context);
}
