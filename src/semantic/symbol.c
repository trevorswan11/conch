#include <assert.h>

#include "semantic/symbol.h"
#include "semantic/type.h"

#include "util/hash.h"

[[nodiscard]] Status symbol_table_create(SymbolTable** table, Allocator* allocator) {
    ASSERT_ALLOCATOR_PTR(allocator);
    SymbolTable* st = ALLOCATOR_PTR_MALLOC(allocator, sizeof(SymbolTable));
    if (!st) { return ALLOCATION_FAILED; }

    HashMap symbols;
    TRY_DO(hash_map_init_allocator(&symbols,
                                   4,
                                   sizeof(MutSlice),
                                   alignof(MutSlice),
                                   sizeof(SemanticType*),
                                   alignof(SemanticType*),
                                   hash_mut_slice,
                                   compare_mut_slices,
                                   allocator),
           ALLOCATOR_PTR_FREE(allocator, st));

    *st = (SymbolTable){
        .symbols = symbols,
    };

    *table = st;
    return SUCCESS;
}

void symbol_table_destroy(SymbolTable* table, Allocator* allocator) {
    if (!table) { return; }
    ASSERT_ALLOCATOR_PTR(allocator);

    MapEntry        next;
    HashMapIterator it = hash_map_iterator_init(&table->symbols);
    while (hash_map_iterator_has_next(&it, &next)) {
        MutSlice* name = (MutSlice*)next.key_ptr;
        if (name->ptr) {
            ALLOCATOR_PTR_FREE(allocator, name->ptr);
            *name = zeroed_mut_slice();
        }

        SemanticType** type = (SemanticType**)next.value_ptr;
        RC_RELEASE(*type, allocator);
    }

    hash_map_deinit(&table->symbols);
    ALLOCATOR_PTR_FREE(allocator, table);
}

[[nodiscard]] Status symbol_table_add(SymbolTable* st, MutSlice symbol, SemanticType* type) {
    assert(st);
    assert(!symbol_table_has(st, symbol));

    const void* retained = rc_retain(type);
    return hash_map_put(&st->symbols, &symbol, (const void*)&retained);
}

[[nodiscard]] Status symbol_table_get(SymbolTable* st, MutSlice symbol, SemanticType** type) {
    assert(st);
    assert(symbol_table_has(st, symbol));

    return hash_map_get_value(&st->symbols, &symbol, (void*)type);
}

bool symbol_table_find(SymbolTable* st, MutSlice symbol, SemanticType** type) {
    assert(st);
    if (!symbol_table_has(st, symbol)) { return false; }

    IGNORE_STATUS(symbol_table_get(st, symbol, type));
    return true;
}

bool symbol_table_has(SymbolTable* st, MutSlice symbol) {
    assert(st);
    return hash_map_contains(&st->symbols, &symbol);
}
