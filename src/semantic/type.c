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

bool semantic_type_is_primitive(SemanticType* type) {
    assert(type);
    switch (type->tag) {
    case STYPE_SIGNED_INTEGER:
    case STYPE_UNSIGNED_INTEGER:
    case STYPE_FLOATING_POINT:
    case STYPE_BYTE_INTEGER:
    case STYPE_STR:
    case STYPE_BOOL:
        return true;
    default:
        return false;
    }
}

bool semantic_type_is_arithmetic(SemanticType* type) {
    assert(type);
    switch (type->tag) {
    case STYPE_SIGNED_INTEGER:
    case STYPE_UNSIGNED_INTEGER:
    case STYPE_FLOATING_POINT:
        return !type->nullable;
    default:
        return false;
    }
}

NODISCARD Status semantic_enum_create(Slice              name,
                                      HashSet            variants,
                                      SemanticEnumType** enum_type,
                                      memory_alloc_fn    memory_alloc) {
    assert(variants.header->key_size == sizeof(MutSlice));
    SemanticEnumType* type_variant = memory_alloc(sizeof(SemanticEnumType));
    if (!type_variant) {
        return ALLOCATION_FAILED;
    }

    *type_variant = (SemanticEnumType){
        .rc_control = rc_init(semantic_enum_destroy), .type_name = name, .variants = variants};

    *enum_type = type_variant;
    return SUCCESS;
}

void free_enum_variant_set(HashSet* variants, free_alloc_fn free_alloc) {
    assert(variants);
    HashSetIterator it = hash_set_iterator_init(variants);
    SetEntry        variant;

    while (hash_set_iterator_has_next(&it, &variant)) {
        MutSlice* name = (MutSlice*)variant.key_ptr;
        free_alloc(name->ptr);
        name->ptr = NULL;
    }

    hash_set_deinit(variants);
}

void semantic_enum_destroy(void* enum_type, free_alloc_fn free_alloc) {
    if (!enum_type) {
        return;
    }
    assert(free_alloc);

    SemanticEnumType* type_variant = (SemanticEnumType*)enum_type;
    free_enum_variant_set(&type_variant->variants, free_alloc);
}

NODISCARD Status semantic_type_create(SemanticType** type, memory_alloc_fn memory_alloc) {
    SemanticType* empty_type = memory_alloc(sizeof(SemanticType));
    if (!empty_type) {
        return ALLOCATION_FAILED;
    }

    *empty_type = (SemanticType){
        .rc_control = rc_init(semantic_type_destroy),
        .is_const   = true,
        .valued     = false,
        .nullable   = false,
    };

    *type = empty_type;
    return SUCCESS;
}

NODISCARD Status semantic_type_copy_variant(SemanticType* dest,
                                            SemanticType* src,
                                            Allocator     allocator) {
    dest->tag = src->tag;
    MAYBE_UNUSED(allocator);

    switch (src->tag) {
    case STYPE_ENUM:
        dest->variant.enum_type = (SemanticEnumType*)rc_retain(src->variant.enum_type);
        break;
    default:
        dest->variant = src->variant;
        break;
    }

    return SUCCESS;
}

NODISCARD Status semantic_type_copy(SemanticType** dest, SemanticType* src, Allocator allocator) {
    SemanticType* type;
    TRY(semantic_type_create(&type, allocator.memory_alloc));

    TRY_DO(semantic_type_copy_variant(type, src, allocator),
           RC_RELEASE(type, allocator.free_alloc));
    type->is_const = src->is_const;
    type->valued   = src->valued;
    type->nullable = src->nullable;

    *dest = type;
    return SUCCESS;
}

void semantic_type_destroy(void* stype, free_alloc_fn free_alloc) {
    if (!stype) {
        return;
    }
    assert(free_alloc);

    SemanticType* type = (SemanticType*)stype;
    switch (type->tag) {
    case STYPE_ENUM:
        RC_RELEASE(type->variant.enum_type, free_alloc);
        break;
    default:
        break;
    }
}

bool type_assignable(const SemanticType* lhs, const SemanticType* rhs) {
    if (rhs->tag == STYPE_NIL) {
        return lhs->nullable;
    }

    if (lhs->nullable && !rhs->nullable) {
        SemanticType tmp = *rhs;
        tmp.nullable     = true;
        return type_equal(lhs, &tmp);
    }

    if (!lhs->nullable && rhs->nullable) {
        return false;
    }

    return type_equal(lhs, rhs);
}

bool type_equal(const SemanticType* lhs, const SemanticType* rhs) {
    // Conch has explicit null values and minimal shallow type checking
    if (lhs->tag != rhs->tag) {
        return false;
    } else if (rhs->tag == STYPE_NIL) {
        return lhs->nullable;
    } else if (lhs->nullable != rhs->nullable) {
        return false;
    }

    // Both the lhs and rhs must be shallowly equal now, try to disprove deep equality
    switch (lhs->tag) {
    case STYPE_ENUM: {
        SemanticEnumType* lhs_enum = lhs->variant.enum_type;
        SemanticEnumType* rhs_enum = rhs->variant.enum_type;
        if (!slice_equals(&lhs_enum->type_name, &rhs_enum->type_name)) {
            return false;
        }

        // The hash sets are equal if they are the same size and one is a subset of the other
        if (lhs_enum->variants.size != rhs_enum->variants.size) {
            return false;
        }

        HashSetIterator it = hash_set_iterator_init(&lhs_enum->variants);
        SetEntry        next;
        while (hash_set_iterator_has_next(&it, &next)) {
            const MutSlice* name = (MutSlice*)next.key_ptr;
            if (!hash_set_contains(&rhs_enum->variants, name)) {
                return false;
            }
        }
        break;
    }
    default:
        break;
    }

    return true;
}
