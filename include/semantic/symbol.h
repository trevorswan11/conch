#pragma once

#include "util/allocator.h"
#include "util/containers/hash_map.h"
#include "util/mem.h"
#include "util/status.h"

typedef Slice               SemanticSymbol;
typedef struct SemanticType SemanticType;

typedef struct SymbolTable {
    HashMap symbols;
} SymbolTable;

NODISCARD Status symbol_table_create(SymbolTable** table, Allocator allocator);
void             symbol_table_destroy(SymbolTable* table, free_alloc_fn free_alloc);

// Adds a symbol/type pair to the table.
NODISCARD Status symbol_table_add(SymbolTable* st, SemanticSymbol symbol, SemanticType type);

bool symbol_table_has(SymbolTable* st, SemanticSymbol symbol);
