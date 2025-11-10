#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "util/containers/array_list.h"
#include "util/mem.h"

typedef struct {
    ArrayList buffer;
} StringBuilder;

bool string_builder_init(StringBuilder* sb, size_t initial_length);
void string_builder_deinit(StringBuilder* sb);
bool string_builder_append(StringBuilder* sb, char byte);
bool string_builder_append_many(StringBuilder* sb, const char* bytes, size_t length);
bool string_builder_append_size(StringBuilder* sb, size_t value);

// Converts the buffer into a null terminated string.
//
// If the result is null, then the buffer failed to append a null byte.
// Otherwise, a string is returned in a slice and must be freed by the caller.
MutSlice string_builder_to_string(StringBuilder* sb);
