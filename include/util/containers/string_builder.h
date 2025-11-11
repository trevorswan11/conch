#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "util/containers/array_list.h"
#include "util/error.h"
#include "util/mem.h"

typedef struct {
    ArrayList buffer;
} StringBuilder;

AnyError string_builder_init(StringBuilder* sb, size_t initial_length);
void     string_builder_deinit(StringBuilder* sb);
AnyError string_builder_append(StringBuilder* sb, char byte);
AnyError string_builder_append_many(StringBuilder* sb, const char* bytes, size_t length);
AnyError string_builder_append_size(StringBuilder* sb, size_t value);

// Converts the buffer into a null terminated string.
//
// In successful calls, a string is returned in a slice and must be freed by the caller.
AnyError string_builder_to_string(StringBuilder* sb, MutSlice* slice);
