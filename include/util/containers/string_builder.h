#ifndef STRING_BUILDER_H
#define STRING_BUILDER_H

#include <stddef.h>
#include <stdint.h>

#include "util/containers/array_list.h"
#include "util/memory.h"
#include "util/status.h"

typedef struct StringBuilder {
    ArrayList buffer;
} StringBuilder;

[[nodiscard]] Status
string_builder_init_allocator(StringBuilder* sb, size_t initial_length, Allocator* allocator);
[[nodiscard]] Status string_builder_init(StringBuilder* sb, size_t initial_length);
void                 string_builder_deinit(StringBuilder* sb);
[[nodiscard]] Status string_builder_append(StringBuilder* sb, char byte);
[[nodiscard]] Status
string_builder_append_many(StringBuilder* sb, const char* bytes, size_t length);
[[nodiscard]] Status string_builder_append_str_z(StringBuilder* sb, const char* str);
[[nodiscard]] Status string_builder_append_slice(StringBuilder* sb, Slice slice);
[[nodiscard]] Status string_builder_append_mut_slice(StringBuilder* sb, MutSlice slice);
[[nodiscard]] Status string_builder_append_unsigned(StringBuilder* sb, uint64_t value);
[[nodiscard]] Status string_builder_append_signed(StringBuilder* sb, int64_t value);

// Converts the buffer into a null terminated string.
//
// In successful calls, a string is returned in a slice and must be freed by the caller.
[[nodiscard]] Status string_builder_to_string(StringBuilder* sb, MutSlice* slice);

#endif
