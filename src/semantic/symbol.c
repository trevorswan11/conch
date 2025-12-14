#include <assert.h>
#include <stdalign.h>

#include "semantic/symbol.h"
#include "semantic/type.h"

#include "util/hash.h"

NODISCARD Status symbol_table_create(SymbolTable** table, Allocator allocator) {
    ASSERT_ALLOCATOR(allocator);
    SymbolTable* st = allocator.memory_alloc(sizeof(SymbolTable));
    if (!st) {
        return ALLOCATION_FAILED;
    }

    HashMap symbols;
    TRY_DO(hash_map_init_allocator(&symbols,
                                   4,
                                   sizeof(MutSlice),
                                   alignof(MutSlice),
                                   sizeof(SemanticType),
                                   alignof(SemanticType),
                                   hash_mut_slice,
                                   compare_mut_slices,
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

    MapEntry        next;
    HashMapIterator it = hash_map_iterator_init(&table->symbols);
    while (hash_map_iterator_has_next(&it, &next)) {
        MutSlice* name = (MutSlice*)next.key_ptr;
        free_alloc(name->ptr);
        SemanticType* type = next.value_ptr;
        semantic_type_deinit(type, free_alloc);
    }

    hash_map_deinit(&table->symbols);
    free_alloc(table);
}

NODISCARD Status symbol_table_add(SymbolTable* st, MutSlice symbol, SemanticType type) {
    assert(st);
    assert(!symbol_table_has(st, symbol));

    return hash_map_put(&st->symbols, &symbol, &type);
}

NODISCARD Status symbol_table_get(SymbolTable* st, MutSlice symbol, SemanticType* type) {
    assert(st);
    assert(symbol_table_has(st, symbol));

    return hash_map_get_value(&st->symbols, &symbol, type);
}

bool symbol_table_find(SymbolTable* st, MutSlice symbol, SemanticType* type) {
    assert(st);
    if (!symbol_table_has(st, symbol)) {
        return false;
    }

    IGNORE_STATUS(hash_map_get_value(&st->symbols, &symbol, type));
    return true;
}

bool symbol_table_has(SymbolTable* st, MutSlice symbol) {
    assert(st);
    return hash_map_contains(&st->symbols, &symbol);
}
