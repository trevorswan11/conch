#ifndef SEMA_CONTEXT_H
#define SEMA_CONTEXT_H

#include "util/memory.h"
#include "util/status.h"

typedef struct SemanticContext SemanticContext;
typedef struct SemanticType    SemanticType;
typedef struct SymbolTable     SymbolTable;

typedef struct SemanticContext {
    SemanticContext* parent;
    SymbolTable*     symbol_table;

    SemanticType* analyzed_type;
    Slice         namespace_type_name;
} SemanticContext;

// Gets a pointer to the context's allocator.
// The context and its symbol table are asserted to be non-null here.
//
// The returned allocator is guaranteed to have a valid vtable.
Allocator*    semantic_context_allocator(SemanticContext* context);
SemanticType* semantic_context_move_analyzed(SemanticContext* context);

[[nodiscard]] Status
semantic_context_create(SemanticContext* parent, SemanticContext** context, Allocator* allocator);

// Releases the provided context but does not wipe parents
void semantic_context_destroy(SemanticContext* context, Allocator* allocator);

bool semantic_context_find(const SemanticContext* context,
                           bool                   check_parents,
                           MutSlice               symbol,
                           SemanticType**         type);
bool semantic_context_has(const SemanticContext* context, bool check_parents, MutSlice symbol);

#endif
