#include <assert.h>
#include <stdalign.h>

#include "semantic/symbol.h"
#include "semantic/type.h"

#include "util/hash.h"
#include "util/status.h"

NODISCARD Status symbol_table_create(SymbolTable** table, Allocator allocator) {
    ASSERT_ALLOCATOR(allocator);
    SymbolTable* st = allocator.memory_alloc(sizeof(SymbolTable));
    if (!st) {
        return ALLOCATION_FAILED;
    }

    HashMap symbols;
    TRY_DO(hash_map_init_allocator(&symbols,
                                   4,
                                   sizeof(SemanticSymbol),
                                   alignof(SemanticSymbol),
                                   sizeof(SemanticType),
                                   alignof(SemanticType),
                                   hash_slice,
                                   compare_slices,
                                   allocator),
           allocator.free_alloc(st));

    *st = (SymbolTable){
        .symbols = symbols,
    };

    *table = st;
    return SUCCESS;
}

void symbol_table_destroy(SymbolTable* table, free_alloc_fn free_alloc) {
    if (!table) {
        return;
    }

    hash_map_deinit(&table->symbols);
    free_alloc(table);
}

NODISCARD Status symbol_table_add(SymbolTable* st, SemanticSymbol symbol, SemanticType type) {
    assert(st);
    if (symbol_table_has(st, symbol)) {
        return ELEMENT_MISSING;
    }

    return hash_map_put(&st->symbols, &symbol, &type);
}

bool symbol_table_has(SymbolTable* st, SemanticSymbol symbol) {
    assert(st);
    return hash_map_contains(&st->symbols, &symbol);
}
