#pragma once

#include <stdbool.h>

#include "util/allocator.h"
#include "util/mem.h"

#define PRIMITIVE_ANALYZE(type)                     \
    assert(node && parent && errors);               \
    MAYBE_UNUSED(node);                             \
    MAYBE_UNUSED(errors);                           \
                                                    \
    parent->analyzed_type.tag      = type;          \
    parent->analyzed_type.variant  = DATALESS_TYPE; \
    parent->analyzed_type.valued   = true;          \
    parent->analyzed_type.nullable = false;         \
    return SUCCESS

typedef enum {
    SIGNED_INTEGER,
    UNSIGNED_INTEGER,
    FLOATING_POINT,
    BYTE_INTEGER,
    STR,
    BOOL,
    VOID,
    NIL_VALUE,
    IMPLICIT_DECLARATION,
} SemanticTypeTag;

// Converts a semantic name to its semantic tag.
//
// This can only return true for primitive types.
bool semantic_name_to_type_tag(MutSlice name, SemanticTypeTag* tag);

typedef struct {
    char _;
} DatalessType;

typedef union {
    DatalessType dataless_type;
} SemanticTypeUnion;

static const SemanticTypeUnion DATALESS_TYPE = {.dataless_type = {'\0'}};

typedef struct SemanticType {
    SemanticTypeTag   tag;
    SemanticTypeUnion variant;
    bool              is_const;
    bool              valued;
    bool              nullable;
} SemanticType;

void semantic_type_deinit(SemanticType* type, free_alloc_fn free_alloc);

// Checks two types against each other, not comparing const-ness
bool type_check(SemanticType a, SemanticType b);
