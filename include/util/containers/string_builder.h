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

TRY_STATUS
string_builder_init_allocator(StringBuilder* sb, size_t initial_length, Allocator allocator);
TRY_STATUS string_builder_init(StringBuilder* sb, size_t initial_length);
void       string_builder_deinit(StringBuilder* sb);
TRY_STATUS string_builder_append(StringBuilder* sb, char byte);
TRY_STATUS string_builder_append_many(StringBuilder* sb, const char* bytes, size_t length);
TRY_STATUS string_builder_append_slice(StringBuilder* sb, Slice slice);
TRY_STATUS string_builder_append_mut_slice(StringBuilder* sb, MutSlice slice);
TRY_STATUS string_builder_append_unsigned(StringBuilder* sb, uint64_t value);
TRY_STATUS string_builder_append_signed(StringBuilder* sb, int64_t value);

// Converts the buffer into a null terminated string.
//
// In successful calls, a string is returned in a slice and must be freed by the caller.
TRY_STATUS string_builder_to_string(StringBuilder* sb, MutSlice* slice);
