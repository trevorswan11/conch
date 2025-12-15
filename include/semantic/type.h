#pragma once

#include <stdbool.h>

#include "util/memory.h"
#include "util/status.h"

#define PRIMITIVE_ANALYZE(T, N)                               \
    assert(node && parent && errors);                         \
    MAYBE_UNUSED(node);                                       \
    MAYBE_UNUSED(errors);                                     \
                                                              \
    SemanticType* type;                                       \
    TRY(semantic_type_create(&type, allocator.memory_alloc)); \
                                                              \
    type->tag      = T;                                       \
    type->variant  = DATALESS_TYPE;                           \
    type->is_const = true;                                    \
    type->valued   = true;                                    \
    type->nullable = N;                                       \
                                                              \
    parent->analyzed_type = type;                             \
    return SUCCESS

typedef enum {
    STYPE_IMPLICIT_DECLARATION,
    STYPE_SIGNED_INTEGER,
    STYPE_UNSIGNED_INTEGER,
    STYPE_FLOATING_POINT,
    STYPE_BYTE_INTEGER,
    STYPE_STR,
    STYPE_BOOL,
    STYPE_VOID,
    STYPE_NIL,
} SemanticTypeTag;

// Converts a semantic name to its semantic tag.
//
// This can only return true for primitive types.
bool semantic_name_to_primitive_type_tag(MutSlice name, SemanticTypeTag* tag);

typedef struct {
    char _;
} DatalessType;

typedef union {
    DatalessType dataless_type;
} SemanticTypeUnion;

static const SemanticTypeUnion DATALESS_TYPE = {.dataless_type = {'\0'}};

typedef struct SemanticType {
    RcControlBlock rc_control;

    SemanticTypeTag   tag;
    SemanticTypeUnion variant;
    bool              is_const;
    bool              valued;
    bool              nullable;
} SemanticType;

// Creates an empty semantic type.
NODISCARD Status semantic_type_create(SemanticType** type, memory_alloc_fn memory_alloc);

// Copies the tagged union data from src to dest, leaving flags alone.
//
// Reference counting is respected when possible.
NODISCARD Status semantic_type_copy_variant(SemanticType*       dest,
                                            const SemanticType* src,
                                            Allocator           allocator);

// Never call this directly!
void semantic_type_destroy(void* type, free_alloc_fn free_alloc);

// Checks two types against each other, not comparing const-ness
bool type_check(SemanticType* lhs, SemanticType* rhs);
