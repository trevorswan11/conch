#pragma once

#include <stdbool.h>
#include <stdint.h>

// An 'owning' dynamic array, data is type erased.
//
// Push uses a memcpy, so non-primitive types, including pointers, require special attention.
typedef struct {
    void*  data;
    size_t item_size;
    size_t capacity;
    size_t length;
} ArrayList;

bool array_list_init(ArrayList* a, size_t capacity, size_t item_size);
void array_list_deinit(ArrayList* a);

bool  array_list_resize(ArrayList* a, size_t new_capacity);
bool  array_list_push(ArrayList* a, void* item);
bool  array_list_pop(ArrayList* a, void* item);
bool  array_list_remove(ArrayList* a, size_t index, void* item);
bool  array_list_remove_item(ArrayList*  a,
                             const void* item,
                             int (*compare)(const void*, const void*));
void* array_list_get(ArrayList* a, size_t index);
bool  array_list_set(ArrayList* a, size_t index, const void* item);

// Finds the element in the array based on the compare function with the following properties:
// - If the first element is larger, return a positive integer
// - If the second element is larger, return a negative integer
// - If the elements are equal, return 0
//
// The input index variable index of the first found element, and false is returned if not found.
bool array_list_find(ArrayList*  a,
                     size_t*     index,
                     const void* item,
                     int (*compare)(const void*, const void*));
