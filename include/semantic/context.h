#pragma once

#include <stdbool.h>

#include "semantic/type.h"

#include "util/allocator.h"
#include "util/mem.h"
#include "util/status.h"

typedef struct SemanticContext SemanticContext;
typedef struct SymbolTable     SymbolTable;

typedef struct SemanticContext {
    SemanticContext* parent;
    SymbolTable*     symbol_table;

    SemanticType analyzed_type;
} SemanticContext;

NODISCARD Status semantic_context_create(SemanticContext*  parent,
                                         SemanticContext** context,
                                         Allocator         allocator);

// Releases the provided context but does not wipe parents
void semantic_context_destroy(SemanticContext* context, free_alloc_fn free_alloc);

bool semantic_context_find(SemanticContext* context,
                           bool             check_parents,
                           MutSlice         symbol,
                           SemanticType*    type);
bool semantic_context_has(SemanticContext* context, bool check_parents, MutSlice symbol);
