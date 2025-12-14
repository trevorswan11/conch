#include <assert.h>
#include <stddef.h>

#include "ast/expressions/type.h"

#include "lexer/token.h"
#include "semantic/type.h"

#include "util/status.h"

#define PRIMITIVE_CASE(tok_type, tag_type) \
    case tok_type: {                       \
        *tag = tag_type;                   \
        return true;                       \
    }

bool semantic_name_to_type_tag(MutSlice name, SemanticTypeTag* tag) {
    const size_t num_primitives = sizeof(ALL_PRIMITIVES) / sizeof(ALL_PRIMITIVES[0]);
    for (size_t i = 0; i < num_primitives; i++) {
        const Slice slice = slice_from_mut(&name);
        if (slice_equals(&slice, &ALL_PRIMITIVES[i].slice)) {
            switch (ALL_PRIMITIVES[i].type) {
                PRIMITIVE_CASE(INT_TYPE, SIGNED_INTEGER)
                PRIMITIVE_CASE(UINT_TYPE, UNSIGNED_INTEGER)
                PRIMITIVE_CASE(FLOAT_TYPE, FLOATING_POINT)
                PRIMITIVE_CASE(BYTE_TYPE, BYTE_INTEGER)
                PRIMITIVE_CASE(STRING_TYPE, STR)
                PRIMITIVE_CASE(BOOL_TYPE, BOOL)
                PRIMITIVE_CASE(VOID_TYPE, VOID)
            default:
                UNREACHABLE;
                return false;
            }
        }
    }

    return false;
}

void semantic_type_deinit(SemanticType* type, free_alloc_fn free_alloc) {
    if (!type) {
        return;
    }
    assert(free_alloc);

    MAYBE_UNUSED(free_alloc);
}

bool type_check(SemanticType a, SemanticType b) { return a.tag == b.tag; }
