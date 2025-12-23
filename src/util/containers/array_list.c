#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "util/containers/array_list.h"
#include "util/math.h"

MAX_FN(size_t, size_t)

NODISCARD Status array_list_init_allocator(ArrayList* a,
                                           size_t     capacity,
                                           size_t     item_size,
                                           Allocator  allocator) {
    ASSERT_ALLOCATOR(allocator);
    if (item_size == 0) { return ZERO_ITEM_SIZE; }
    void* data = allocator.memory_alloc(capacity * item_size);
    if (!data && capacity > 0) { return ALLOCATION_FAILED; }

    *a = (ArrayList){
        .data      = data,
        .item_size = item_size,
        .capacity  = capacity,
        .length    = 0,
        .allocator = allocator,
    };
    return SUCCESS;
}

NODISCARD Status array_list_init(ArrayList* a, size_t capacity, size_t item_size) {
    return array_list_init_allocator(a, capacity, item_size, STANDARD_ALLOCATOR);
}

void array_list_deinit(ArrayList* a) {
    if (!a || !a->data) { return; }
    ASSERT_ALLOCATOR(a->allocator);

    a->allocator.free_alloc(a->data);
    a->data     = NULL;
    a->length   = 0;
    a->capacity = 0;
}

size_t array_list_capacity(const ArrayList* a) {
    assert(a && a->data);
    return a->capacity;
}

size_t array_list_length(const ArrayList* a) {
    assert(a && a->data);
    return a->length;
}

NODISCARD Status array_list_resize(ArrayList* a, size_t new_capacity) {
    assert(a);
    ASSERT_ALLOCATOR(a->allocator);

    if (!a) { return NULL_PARAMETER; }
    if (new_capacity == 0) {
        array_list_deinit(a);
        return SUCCESS;
    }

    void* new_data = a->allocator.re_alloc(a->data, new_capacity * a->item_size);
    if (!new_data) { return REALLOCATION_FAILED; }

    a->data     = new_data;
    a->capacity = new_capacity;

    if (a->length > new_capacity) { a->length = new_capacity; }
    return SUCCESS;
}

NODISCARD Status array_list_ensure_total_capacity(ArrayList* a, size_t new_capacity) {
    assert(a && a->data);
    if (a->capacity < new_capacity) { return array_list_resize(a, new_capacity); }
    return SUCCESS;
}

void array_list_clear_retaining_capacity(ArrayList* a) {
    assert(a && a->data);
    a->length = 0;
}

NODISCARD Status array_list_shrink_to_fit(ArrayList* a) {
    assert(a && a->data);
    if (a->length < a->capacity) { return array_list_resize(a, a->length); }
    return SUCCESS;
}

static inline void* array_list_get_ptr_unsafe(ArrayList* a, size_t index) {
    assert(a && a->data);
    return ptr_offset(a->data, index * a->item_size);
}

static inline const void* array_list_get_unsafe(const ArrayList* a, size_t index) {
    assert(a && a->data);
    return ptr_offset(a->data, index * a->item_size);
}

NODISCARD Status array_list_push(ArrayList* a, const void* item) {
    assert(a && a->data);

    if (a->length == a->capacity) { TRY(array_list_resize(a, max_size_t(2, a->capacity * 2, 4))); }

    void* dest = array_list_get_ptr_unsafe(a, a->length);
    memcpy(dest, item, a->item_size);
    a->length += 1;
    return SUCCESS;
}

void array_list_push_assume_capacity(ArrayList* a, const void* item) {
    assert(a && a->data && a->length < a->capacity);
    void* dest = array_list_get_ptr_unsafe(a, a->length);
    memcpy(dest, item, a->item_size);
    a->length += 1;
}

NODISCARD Status array_list_insert_stable(ArrayList* a, size_t index, const void* item) {
    assert(a && a->data);
    assert(index <= a->length);

    if (a->length == a->capacity) { TRY(array_list_resize(a, max_size_t(2, a->capacity * 2, 4))); }

    array_list_insert_stable_assume_capacity(a, index, item);
    return SUCCESS;
}

void array_list_insert_stable_assume_capacity(ArrayList* a, size_t index, const void* item) {
    assert(a && a->data);
    assert(index <= a->length);

    void* dest       = array_list_get_ptr_unsafe(a, index);
    void* shift_dest = array_list_get_ptr_unsafe(a, index + 1);
    memmove(shift_dest, dest, (a->length - index) * a->item_size);

    memcpy(dest, item, a->item_size);
    a->length += 1;
}

NODISCARD Status array_list_insert_unstable(ArrayList* a, size_t index, const void* item) {
    assert(a && a->data);
    assert(index <= a->length);

    TRY(array_list_push(a, item));

    const size_t back = a->length - 1;
    if (index != back) {
        void* current = array_list_get_ptr_unsafe(a, index);
        void* new     = array_list_get_ptr_unsafe(a, a->length - 1);
        swap(current, new, a->item_size);
    }
    return SUCCESS;
}

void array_list_insert_unstable_assume_capacity(ArrayList* a, size_t index, const void* item) {
    assert(a && a->data);
    assert(index <= a->length);

    array_list_push_assume_capacity(a, item);
    const size_t back = a->length - 1;

    if (index != back) {
        void* current = array_list_get_ptr_unsafe(a, index);
        void* new     = array_list_get_ptr_unsafe(a, a->length - 1);
        swap(current, new, a->item_size);
    }
}

NODISCARD Status array_list_pop(ArrayList* a, void* item) {
    assert(a && a->data);
    if (a->length == 0) { return EMPTY; }

    void* last = array_list_get_ptr_unsafe(a, a->length - 1);
    memcpy(item, last, a->item_size);
    a->length -= 1;
    return SUCCESS;
}

NODISCARD Status array_list_remove(ArrayList* a, size_t index, void* item) {
    assert(a && a->data);
    if (index >= a->length) { return INDEX_OUT_OF_BOUNDS; }

    const void* at = array_list_get_ptr_unsafe(a, index);
    if (item) { memcpy(item, at, a->item_size); }

    for (size_t i = index; i < a->length - 1; i++) {
        const void* src  = array_list_get_ptr_unsafe(a, i + 1);
        void*       dest = array_list_get_ptr_unsafe(a, i);
        memcpy(dest, src, a->item_size);
    }

    a->length -= 1;
    return SUCCESS;
}

NODISCARD Status array_list_remove_item(ArrayList*  a,
                                        const void* item,
                                        int (*compare)(const void*, const void*)) {
    size_t index;
    TRY(array_list_find(a, &index, item, compare));
    return array_list_remove(a, index, NULL);
}

NODISCARD Status array_list_get(const ArrayList* a, size_t index, void* item) {
    assert(a && a->data);
    if (index >= a->length) { return INDEX_OUT_OF_BOUNDS; }

    memcpy(item, array_list_get_unsafe(a, index), a->item_size);
    return SUCCESS;
}

NODISCARD Status array_list_get_ptr(ArrayList* a, size_t index, void** item) {
    assert(a && a->data);
    if (index >= a->length) { return INDEX_OUT_OF_BOUNDS; }

    *item = array_list_get_ptr_unsafe(a, index);
    return SUCCESS;
}

NODISCARD Status array_list_set(ArrayList* a, size_t index, const void* item) {
    assert(a && a->data);
    if (index >= a->length) { return INDEX_OUT_OF_BOUNDS; }

    void* dest = array_list_get_ptr_unsafe(a, index);
    memcpy(dest, item, a->item_size);
    return SUCCESS;
}

NODISCARD Status array_list_find(const ArrayList* a,
                                 size_t*          index,
                                 const void*      item,
                                 int (*compare)(const void*, const void*)) {
    assert(a && a->data);
    if (!index || !item || !compare) { return NULL_PARAMETER; }

    for (size_t i = 0; i < a->length; i++) {
        if (compare(item, array_list_get_unsafe(a, i)) == 0) {
            *index = i;
            return SUCCESS;
        }
    }
    return ELEMENT_MISSING;
}

bool array_list_bsearch(const ArrayList* a,
                        size_t*          index,
                        const void*      item,
                        int (*compare)(const void*, const void*)) {
    assert(a && a->data && item && compare);
    size_t low  = 0;
    size_t high = a->length;

    while (low < high) {
        size_t      mid        = low + ((high - low) / 2);
        const void* elem       = array_list_get_unsafe(a, mid);
        const int   comparison = compare(elem, item);

        if (comparison == 0) {
            *index = mid;
            return true;
        }

        if (comparison < 0) {
            low = mid + 1;
        } else {
            high = mid;
        }
    }

    *index = low;
    return false;
}

void array_list_sort(ArrayList* a, int (*compare)(const void*, const void*)) {
    assert(a && a->data && compare);
    qsort(a->data, a->length, a->item_size, compare);
}

bool array_list_is_sorted(const ArrayList* a, int (*compare)(const void*, const void*)) {
    assert(a && a->data && compare);
    if (a->length == 0) { return true; }

    for (size_t i = 0; i < a->length - 1; i++) {
        const void* curr = array_list_get_unsafe(a, i);
        const void* next = array_list_get_unsafe(a, i + 1);
        if (compare(curr, next) > 0) { return false; }
    }

    return true;
}

ArrayListIterator array_list_iterator_init(ArrayList* a) {
    assert(a && a->data);
    return (ArrayListIterator){
        .a     = a,
        .index = 0,
    };
}

bool array_list_iterator_has_next(ArrayListIterator* it, void* next) {
    assert(it && it->a && it->a->data);
    return STATUS_OK(array_list_get(it->a, it->index++, next));
}

bool array_list_iterator_exhausted(const ArrayListIterator* it) {
    assert(it && it->a && it->a->data);
    return it->index >= it->a->length;
}

ArrayListConstIterator array_list_const_iterator_init(const ArrayList* a) {
    assert(a && a->data);
    return (ArrayListConstIterator){
        .a     = a,
        .index = 0,
    };
}

bool array_list_const_iterator_has_next(ArrayListConstIterator* it, void* next) {
    assert(it && it->a && it->a->data);
    return STATUS_OK(array_list_get(it->a, it->index++, next));
}
