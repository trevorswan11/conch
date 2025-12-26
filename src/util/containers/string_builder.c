#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>

#include "util/containers/string_builder.h"

NODISCARD Status string_builder_init_allocator(StringBuilder* sb,
                                               size_t         initial_length,
                                               Allocator      allocator) {
    if (!sb) { return NULL_PARAMETER; }
    return initial_length == 0
               ? EMPTY
               : array_list_init_allocator(&sb->buffer, initial_length, sizeof(char), allocator);
}

NODISCARD Status string_builder_init(StringBuilder* sb, size_t initial_length) {
    return string_builder_init_allocator(sb, initial_length, STANDARD_ALLOCATOR);
}

void string_builder_deinit(StringBuilder* sb) {
    if (!sb) { return; }
    array_list_deinit(&sb->buffer);
}

NODISCARD Status string_builder_append(StringBuilder* sb, char byte) {
    assert(sb);
    return array_list_push(&sb->buffer, &byte);
}

NODISCARD Status string_builder_append_many(StringBuilder* sb, const char* bytes, size_t length) {
    assert(sb);
    if (!bytes) { return NULL_PARAMETER; }

    TRY(array_list_ensure_total_capacity(&sb->buffer, sb->buffer.length + length));

    for (size_t i = 0; i < length; i++) {
        array_list_push_assume_capacity(&sb->buffer, &bytes[i]);
    }
    return SUCCESS;
}

NODISCARD Status string_builder_append_str_z(StringBuilder* sb, const char* str) {
    return string_builder_append_many(sb, str, strlen(str));
}

NODISCARD Status string_builder_append_slice(StringBuilder* sb, Slice slice) {
    TRY(string_builder_append_many(sb, slice.ptr, slice.length));
    return SUCCESS;
}

NODISCARD Status string_builder_append_mut_slice(StringBuilder* sb, MutSlice slice) {
    TRY(string_builder_append_many(sb, slice.ptr, slice.length));
    return SUCCESS;
}

NODISCARD Status string_builder_append_unsigned(StringBuilder* sb, uint64_t value) {
    assert(sb);

    // Use a copy to determine the total number of digits to reserve
    uint64_t temp   = value;
    uint64_t digits = 0;
    if (temp == 0) {
        digits = 1;
    } else {
        while (temp > 0) {
            temp /= 10;
            digits++;
        }
    }

    TRY(array_list_ensure_total_capacity(&sb->buffer, sb->buffer.length + digits));

    // Now we can push in reverse order without further allocations
    uint64_t div = 1;
    for (size_t i = 1; i < digits; i++) {
        div *= 10;
    }

    for (size_t i = 0; i < digits; i++) {
        char digit = (char)((uint64_t)('0') + (value / div % 10));
        array_list_push_assume_capacity(&sb->buffer, &digit);
        div /= 10;
    }

    return SUCCESS;
}

NODISCARD Status string_builder_append_signed(StringBuilder* sb, int64_t value) {
    char   buffer[32];
    size_t written = snprintf(buffer, sizeof(buffer), "%" PRId64, value);
    if (written > sizeof(buffer)) { return BUFFER_OVERFLOW; }

    TRY(string_builder_append_many(sb, buffer, written));
    return SUCCESS;
}

NODISCARD Status string_builder_to_string(StringBuilder* sb, MutSlice* slice) {
    assert(sb && slice);

    const char null_byte = '\0';
    TRY(array_list_push(&sb->buffer, &null_byte));
    TRY(array_list_shrink_to_fit(&sb->buffer));

    *slice = (MutSlice){
        .ptr    = sb->buffer.data,
        .length = sb->buffer.length - 1,
    };
    return SUCCESS;
}
