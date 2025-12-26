#ifndef SEMA_SYMBOL_H
#define SEMA_SYMBOL_H

#include "util/containers/hash_map.h"
#include "util/memory.h"
#include "util/status.h"

typedef struct SemanticType SemanticType;

typedef struct SymbolTable {
    HashMap symbols;
} SymbolTable;

[[nodiscard]] Status symbol_table_create(SymbolTable** table, Allocator allocator);
void             symbol_table_destroy(SymbolTable* table, free_alloc_fn free_alloc);

// Adds a symbol/type pair to the table.
// The type is retained by the table, so it can never be released if the table holds it.
//
// Asserts that the symbol is not already present in the table.
[[nodiscard]] Status symbol_table_add(SymbolTable* st, MutSlice symbol, SemanticType* type);

// Gets a type from the table.
//
// Asserts that the symbol is already present in the table.
[[nodiscard]] Status symbol_table_get(SymbolTable* st, MutSlice symbol, SemanticType** type);

// Gets a type from the table. Returning a boolean indicating if the symbol was found.
bool symbol_table_find(SymbolTable* st, MutSlice symbol, SemanticType** type);

bool symbol_table_has(SymbolTable* st, MutSlice symbol);

#endif
