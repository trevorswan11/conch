#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "util/allocator.h"
#include "util/containers/array_list.h"
#include "util/mem.h"
#include "util/status.h"

typedef struct {
    ArrayList buffer;
} StringBuilder;

NODISCARD Status string_builder_init_allocator(StringBuilder* sb,
                                               size_t         initial_length,
                                               Allocator      allocator);
NODISCARD Status string_builder_init(StringBuilder* sb, size_t initial_length);
void             string_builder_deinit(StringBuilder* sb);
NODISCARD Status string_builder_append(StringBuilder* sb, char byte);
NODISCARD Status string_builder_append_many(StringBuilder* sb, const char* bytes, size_t length);
NODISCARD Status string_builder_append_str_z(StringBuilder* sb, const char* str);
NODISCARD Status string_builder_append_slice(StringBuilder* sb, Slice slice);
NODISCARD Status string_builder_append_mut_slice(StringBuilder* sb, MutSlice slice);
NODISCARD Status string_builder_append_unsigned(StringBuilder* sb, uint64_t value);
NODISCARD Status string_builder_append_signed(StringBuilder* sb, int64_t value);

// Converts the buffer into a null terminated string.
//
// In successful calls, a string is returned in a slice and must be freed by the caller.
NODISCARD Status string_builder_to_string(StringBuilder* sb, MutSlice* slice);
