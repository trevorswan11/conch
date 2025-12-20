#pragma once

#include <stdbool.h>

#include "util/containers/hash_set.h"
#include "util/memory.h"
#include "util/status.h"

typedef struct SemanticType SemanticType;

#define MAKE_PRIMITIVE(T, N, name, memory_alloc, err)       \
    SemanticType* name;                                     \
    TRY_DO(semantic_type_create(&name, memory_alloc), err); \
                                                            \
    name->tag      = T;                                     \
    name->variant  = SEMANTIC_DATALESS_TYPE;                \
    name->is_const = true;                                  \
    name->valued   = true;                                  \
    name->nullable = N;

#define PRIMITIVE_ANALYZE(T, N, A)    \
    ASSERT_EXPRESSION(node);          \
    assert(parent);                   \
    assert(errors);                   \
    MAYBE_UNUSED(node);               \
    MAYBE_UNUSED(errors);             \
    MAKE_PRIMITIVE(T, N, type, A, {}) \
                                      \
    parent->analyzed_type = type;     \
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
    STYPE_ENUM,
} SemanticTypeTag;

// Converts a semantic name to its semantic tag.
//
// This can only return true for primitive types.
bool semantic_name_to_primitive_type_tag(MutSlice name, SemanticTypeTag* tag);

// Checks if a type is a 'valued' primitive (i.e not void or nil)
bool semantic_type_is_primitive(SemanticType* type);

// Checks if a type is trivially arithmetic (i.e not nullable or a byte)
bool semantic_type_is_arithmetic(SemanticType* type);

typedef struct {
    char _;
} SematicDatalessType;

typedef struct {
    RcControlBlock rc_control;

    Slice   type_name;
    HashSet variants;
} SemanticEnumType;

NODISCARD Status semantic_enum_create(Slice              name,
                                      HashSet            variants,
                                      SemanticEnumType** enum_type,
                                      memory_alloc_fn    memory_alloc);
void             free_enum_variant_set(HashSet* variants, free_alloc_fn free_alloc);
void             semantic_enum_destroy(void* enum_type, free_alloc_fn free_alloc);

typedef union {
    SematicDatalessType dataless_type;
    SemanticEnumType*   enum_type;
} SemanticTypeUnion;

static const SemanticTypeUnion SEMANTIC_DATALESS_TYPE = {.dataless_type = {'\0'}};

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
NODISCARD Status semantic_type_copy_variant(SemanticType* dest,
                                            SemanticType* src,
                                            Allocator     allocator);

// Creates and deep copies src into dest while respecting RC.
NODISCARD Status semantic_type_copy(SemanticType** dest, SemanticType* src, Allocator allocator);

// Never call this directly!
void semantic_type_destroy(void* stype, free_alloc_fn free_alloc);

// Checks if a type of rhs can be assigned to a type lhs
bool type_assignable(const SemanticType* lhs, const SemanticType* rhs);

// Checks two types against each other, not comparing const-ness
bool type_equal(const SemanticType* lhs, const SemanticType* rhs);
