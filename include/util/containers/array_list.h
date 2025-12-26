#ifndef ARRAY_LIST_H
#define ARRAY_LIST_H

#include <stddef.h>
#include <stdint.h>

#include "util/memory.h"
#include "util/status.h"

// An 'owning' dynamic array, data is type erased.
//
// Push uses a memcpy, so non-trivial types require special attention.
typedef struct ArrayList {
    void*  data;
    size_t item_size;
    size_t capacity;
    size_t length;

    Allocator allocator;
} ArrayList;

// A non-owning iterator. Invalid if the underlying array is freed.
// An iterator is invalidated if it modifies the array during iteration.
typedef struct {
    ArrayList* a;
    size_t     index;
} ArrayListIterator;

typedef struct {
    const ArrayList* a;
    size_t           index;
} ArrayListConstIterator;

[[nodiscard]] Status array_list_init_allocator(ArrayList* a,
                                           size_t     capacity,
                                           size_t     item_size,
                                           Allocator  allocator);
[[nodiscard]] Status array_list_init(ArrayList* a, size_t capacity, size_t item_size);
void             array_list_deinit(ArrayList* a);

size_t array_list_capacity(const ArrayList* a);
size_t array_list_length(const ArrayList* a);

// Resizes the array, growing/shrinking based on the new capacity.
//
// If the new capacity is 0, this is equivalent to a deinit call.
[[nodiscard]] Status array_list_resize(ArrayList* a, size_t new_capacity);

// Resizes the array if and only if the new capacity exceeds the current capacity.
//
// If the array does not need to be resized, then true is returned.
[[nodiscard]] Status array_list_ensure_total_capacity(ArrayList* a, size_t new_capacity);
void             array_list_clear_retaining_capacity(ArrayList* a);
[[nodiscard]] Status array_list_shrink_to_fit(ArrayList* a);

// Inserts an item at the given position, maintaining relative order of the two regions.
//
// If the index given is at the end of the list, this is equivalent to push.
[[nodiscard]] Status array_list_insert_stable(ArrayList* a, size_t index, const void* item);
void array_list_insert_stable_assume_capacity(ArrayList* a, size_t index, const void* item);

// Inserts an item at the given position, invalidating relative order.
//
// If the index given is at the end of the list, this is equivalent to push.
[[nodiscard]] Status array_list_insert_unstable(ArrayList* a, size_t index, const void* item);
void array_list_insert_unstable_assume_capacity(ArrayList* a, size_t index, const void* item);

[[nodiscard]] Status array_list_push(ArrayList* a, const void* item);
void             array_list_push_assume_capacity(ArrayList* a, const void* item);
[[nodiscard]] Status array_list_pop(ArrayList* a, void* item);
[[nodiscard]] Status array_list_remove(ArrayList* a, size_t index, void* item);
[[nodiscard]] Status array_list_remove_item(ArrayList*  a,
                                        const void* item,
                                        int (*compare)(const void*, const void*));
[[nodiscard]] Status array_list_get(const ArrayList* a, size_t index, void* item);
[[nodiscard]] Status array_list_get_ptr(ArrayList* a, size_t index, void** item);
[[nodiscard]] Status array_list_set(ArrayList* a, size_t index, const void* item);

// Finds the element in the array based on the compare function with the following properties:
// - If the first element is larger, return a positive integer
// - If the second element is larger, return a negative integer
// - If the elements are equal, return 0
//
// The input index variable index of the first found element, and false is returned if not found.
[[nodiscard]] Status array_list_find(const ArrayList* a,
                                 size_t*          index,
                                 const void*      item,
                                 int (*compare)(const void*, const void*));

// Performs binary search on the array, setting the index to the real position if found.
// If the element is not found, then the index is set to its insertion position.
//
// The compare function should align with that used to sort the array:
// - If the first element is of higher priority, return a positive integer
// - If the second element is of higher priority, return a negative integer
// - If the elements are equal, return 0
//
// Generally, calling find will be faster in this scenario
bool array_list_bsearch(const ArrayList* a,
                        size_t*          index,
                        const void*      item,
                        int (*compare)(const void*, const void*));

// Performs an in-place sort using quicksort.
// - If the first element is of higher priority, return a positive integer
// - If the second element is of higher priority, return a negative integer
// - If the elements are equal, return 0
//
// The array is sorted in ascending order with respect to the first parameter in the compare
// function.
void array_list_sort(ArrayList* a, int (*compare)(const void*, const void*));

// Check if the array is sorted with respect to the compare function:
// - If the first element is of higher priority, return a positive integer
// - If the second element is of higher priority, return a negative integer
// - If the elements are equal, return 0
//
// For the purposes of this function, an array is sorted if, for every element i, it is less than or
// equal to all succeeding elements j.
bool array_list_is_sorted(const ArrayList* a, int (*compare)(const void*, const void*));

ArrayListIterator array_list_iterator_init(ArrayList* a);
bool              array_list_iterator_has_next(ArrayListIterator* it, void** next);
bool              array_list_iterator_exhausted(const ArrayListIterator* it);

ArrayListConstIterator array_list_const_iterator_init(const ArrayList* a);
bool                   array_list_const_iterator_has_next(ArrayListConstIterator* it, void* next);
bool                   array_list_const_iterator_exhausted(const ArrayListConstIterator* it);

#endif
