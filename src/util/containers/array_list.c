#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "util/containers/array_list.h"
#include "util/math.h"
#include "util/mem.h"

MAX_FN(size_t, size_t)

bool array_list_init(ArrayList* a, size_t capacity, size_t item_size) {
    void* data = malloc(capacity * item_size);
    if (!data && capacity > 0) {
        return false;
    }

    *a = (ArrayList){
        .data      = data,
        .item_size = item_size,
        .capacity  = capacity,
        .length    = 0,
    };
    return true;
}

void array_list_deinit(ArrayList* a) {
    if (!a || !a->data) {
        return;
    }

    free(a->data);
    a->data     = NULL;
    a->length   = 0;
    a->capacity = 0;
}

size_t array_list_capacity(const ArrayList* a) {
    if (!a || !a->data) {
        return 0;
    }
    return a->capacity;
}

size_t array_list_length(const ArrayList* a) {
    if (!a || !a->data) {
        return 0;
    }
    return a->length;
}

bool array_list_resize(ArrayList* a, size_t new_capacity) {
    if (!a) {
        return false;
    } else if (new_capacity == 0) {
        array_list_deinit(a);
        return true;
    }

    void* new_data = realloc(a->data, new_capacity * a->item_size);
    if (!new_data) {
        return false;
    }

    a->data     = new_data;
    a->capacity = new_capacity;

    if (a->length > new_capacity) {
        a->length = new_capacity;
    }
    return true;
}

bool array_list_ensure_total_capacity(ArrayList* a, size_t new_capacity) {
    assert(a && a->data && new_capacity > 0);
    if (a->capacity < new_capacity) {
        return array_list_resize(a, new_capacity);
    }
    return true;
}

static inline void* _array_list_get_ptr_unsafe(ArrayList* a, size_t index) {
    assert(a && a->data);
    return ptr_offset(a->data, index * a->item_size);
}

static inline const void* _array_list_get_unsafe(const ArrayList* a, size_t index) {
    assert(a && a->data);
    return ptr_offset(a->data, index * a->item_size);
}

bool array_list_push(ArrayList* a, const void* item) {
    assert(a && a->data);

    if (a->length == a->capacity) {
        if (!array_list_resize(a, max_size_t(2, a->capacity * 2, 4))) {
            return false;
        }
    }

    void* dest = _array_list_get_ptr_unsafe(a, a->length);
    memcpy(dest, item, a->item_size);
    a->length += 1;
    return true;
}

bool array_list_insert_stable(ArrayList* a, size_t index, const void* item) {
    assert(a && a->data);
    assert(index <= a->length);

    if (a->length == a->capacity) {
        if (!array_list_resize(a, max_size_t(2, a->capacity * 2, 4))) {
            return false;
        }
    }

    void* dest       = _array_list_get_ptr_unsafe(a, index);
    void* shift_dest = _array_list_get_ptr_unsafe(a, index + 1);
    memmove(shift_dest, dest, (a->length - index) * a->item_size);

    memcpy(dest, item, a->item_size);
    a->length += 1;
    return true;
}

bool array_list_insert_unstable(ArrayList* a, size_t index, const void* item) {
    assert(a && a->data);
    assert(index <= a->length);

    if (!array_list_push(a, item)) {
        return false;
    }

    const size_t back = a->length - 1;
    if (index != back) {
        void* current = _array_list_get_ptr_unsafe(a, index);
        void* new     = _array_list_get_ptr_unsafe(a, a->length - 1);
        swap(current, new, a->item_size);
    }
    return true;
}

bool array_list_pop(ArrayList* a, void* item) {
    assert(a && a->data);
    if (a->length == 0) {
        return false;
    }

    void* last = _array_list_get_ptr_unsafe(a, a->length - 1);
    memcpy(item, last, a->item_size);
    a->length -= 1;
    return true;
}

bool array_list_remove(ArrayList* a, size_t index, void* item) {
    assert(a && a->data);
    if (index >= a->length) {
        return false;
    }

    const void* at = _array_list_get_ptr_unsafe(a, index);
    if (item) {
        memcpy(item, at, a->item_size);
    }

    for (size_t i = index; i < a->length - 1; i++) {
        const void* src  = _array_list_get_ptr_unsafe(a, i + 1);
        void*       dest = _array_list_get_ptr_unsafe(a, i);
        memcpy(dest, src, a->item_size);
    }

    a->length -= 1;
    return true;
}

bool array_list_remove_item(ArrayList*  a,
                            const void* item,
                            int (*compare)(const void*, const void*)) {
    size_t index;
    if (array_list_find(a, &index, item, compare)) {
        return array_list_remove(a, index, NULL);
    } else {
        return false;
    }
}

bool array_list_get(const ArrayList* a, size_t index, void* item) {
    assert(a && a->data);
    if (index >= a->length) {
        return false;
    }

    memcpy(item, _array_list_get_unsafe(a, index), a->item_size);
    return true;
}

void* array_list_get_ptr(ArrayList* a, size_t index) {
    assert(a && a->data);
    if (index >= a->length) {
        return NULL;
    }

    return _array_list_get_ptr_unsafe(a, index);
}

bool array_list_set(ArrayList* a, size_t index, const void* item) {
    assert(a && a->data);
    if (index >= a->length) {
        return false;
    }

    void* dest = _array_list_get_ptr_unsafe(a, index);
    memcpy(dest, item, a->item_size);
    return true;
}

bool array_list_find(const ArrayList* a,
                     size_t*          index,
                     const void*      item,
                     int (*compare)(const void*, const void*)) {
    assert(a && a->data);
    if (!index || !item || !compare) {
        return false;
    }

    for (size_t i = 0; i < a->length; i++) {
        if (compare(item, _array_list_get_unsafe(a, i)) == 0) {
            *index = i;
            return true;
        }
    }
    return false;
}

bool array_list_bsearch(const ArrayList* a,
                        size_t*          index,
                        const void*      item,
                        int (*compare)(const void*, const void*)) {
    assert(a && a->data && item && compare);
    size_t low = 0, high = a->length;

    while (low < high) {
        size_t      mid        = low + (high - low) / 2;
        const void* elem       = _array_list_get_unsafe(a, mid);
        const int   comparison = compare(elem, item);

        if (comparison == 0) {
            *index = mid;
            return true;
        } else if (comparison < 0) {
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
    if (a->length == 0) {
        return true;
    }

    for (size_t i = 0; i < a->length - 1; i++) {
        const void* curr = _array_list_get_unsafe(a, i);
        const void* next = _array_list_get_unsafe(a, i + 1);
        if (compare(curr, next) > 0) {
            return false;
        }
    }

    return true;
}
