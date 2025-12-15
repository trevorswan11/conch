#include <assert.h>
#include <stddef.h>

#include "lexer/token.h"

#include "ast/expressions/type.h"

#include "semantic/type.h"

#define PRIMITIVE_CASE(tok_type, tag_type) \
    case tok_type: {                       \
        *tag = tag_type;                   \
        return true;                       \
    }

bool semantic_name_to_primitive_type_tag(MutSlice name, SemanticTypeTag* tag) {
    const size_t num_primitives = sizeof(ALL_PRIMITIVES) / sizeof(ALL_PRIMITIVES[0]);
    for (size_t i = 0; i < num_primitives; i++) {
        const Slice slice = slice_from_mut(&name);
        if (slice_equals(&slice, &ALL_PRIMITIVES[i].slice)) {
            switch (ALL_PRIMITIVES[i].type) {
                PRIMITIVE_CASE(INT_TYPE, STYPE_SIGNED_INTEGER)
                PRIMITIVE_CASE(UINT_TYPE, STYPE_UNSIGNED_INTEGER)
                PRIMITIVE_CASE(FLOAT_TYPE, STYPE_FLOATING_POINT)
                PRIMITIVE_CASE(BYTE_TYPE, STYPE_BYTE_INTEGER)
                PRIMITIVE_CASE(STRING_TYPE, STYPE_STR)
                PRIMITIVE_CASE(BOOL_TYPE, STYPE_BOOL)
                PRIMITIVE_CASE(VOID_TYPE, STYPE_VOID)
            default:
                UNREACHABLE;
                return false;
            }
        }
    }

    return false;
}

NODISCARD Status semantic_type_create(SemanticType** type, memory_alloc_fn memory_alloc) {
    SemanticType* empty_type = memory_alloc(sizeof(SemanticType));
    if (!empty_type) {
        return ALLOCATION_FAILED;
    }

    *empty_type = (SemanticType){
        .rc_control = rc_init(&semantic_type_destroy),
        .is_const   = true,
        .nullable   = false,
        .valued     = false,
    };

    *type = empty_type;
    return SUCCESS;
}

NODISCARD Status semantic_type_copy_variant(SemanticType*       dest,
                                            const SemanticType* src,
                                            Allocator           allocator) {
    dest->tag     = src->tag;
    dest->variant = src->variant;
    MAYBE_UNUSED(allocator);

    switch (src->tag) {
    case STYPE_SIGNED_INTEGER:
    case STYPE_UNSIGNED_INTEGER:
    case STYPE_FLOATING_POINT:
    case STYPE_BYTE_INTEGER:
    case STYPE_STR:
    case STYPE_BOOL:
        break;
    default:
        return NOT_IMPLEMENTED;
    }

    return SUCCESS;
}

void semantic_type_destroy(void* type, free_alloc_fn free_alloc) {
    if (!type) {
        return;
    }
    assert(free_alloc);
    MAYBE_UNUSED(free_alloc);
}

bool type_check(SemanticType* lhs, SemanticType* rhs) {
    // Conch has explicit null values
    if (lhs->nullable && rhs->nullable) {
        return true;
    }

    return lhs->tag == rhs->tag;
}
