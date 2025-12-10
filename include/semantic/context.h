#pragma once

#include "util/allocator.h"
#include "util/containers/hash_map.h"
#include "util/status.h"

typedef struct SemanticContext SemanticContext;

typedef struct SemanticContext {
    SemanticContext* parent;
    HashMap          symbol_table;
    Allocator        allocator;
} SemanticContext;

NODISCARD Status semantic_context_create(SemanticContext*  parent,
                                         SemanticContext** context,
                                         Allocator         allocator);

// Releases the current allocator but does not wipe parents
void semantic_context_destroy(SemanticContext* context);
void semantic_context_clear_lineage(SemanticContext* context);
