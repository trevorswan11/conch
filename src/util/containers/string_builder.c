#include <stdbool.h>
#include <stddef.h>

#include "util/containers/array_list.h"
#include "util/containers/string_builder.h"
#include "util/mem.h"

bool string_builder_init(StringBuilder* sb, size_t initial_length) {
    if (!sb || initial_length == 0) {
        return false;
    }

    return array_list_init(&sb->buffer, initial_length, sizeof(char));
}

void string_builder_deinit(StringBuilder* sb) {
    if (!sb) {
        return;
    }

    array_list_deinit(&sb->buffer);
}

bool string_builder_append(StringBuilder* sb, char byte) {
    assert(sb);
    return array_list_push(&sb->buffer, &byte);
}

bool string_builder_append_many(StringBuilder* sb, const char* bytes, size_t length) {
    assert(sb);

    if (!bytes) {
        return false;
    }
    if (!array_list_ensure_total_capacity(&sb->buffer, sb->buffer.length + length)) {
        return false;
    }

    for (size_t i = 0; i < length; i++) {
        array_list_push_assume_capacity(&sb->buffer, &bytes[i]);
    }
    return true;
}

bool string_builder_append_size(StringBuilder* sb, size_t value) {
    assert(sb);

    // Use a copy to determine the total number of digits to reserve
    size_t temp   = value;
    size_t digits = 0;
    if (temp == 0) {
        digits = 1;
    } else {
        while (temp > 0) {
            temp /= 10;
            digits++;
        }
    }

    if (!array_list_ensure_total_capacity(&sb->buffer, sb->buffer.length + digits)) {
        return false;
    }

    // Now we can push in reverse order without further allocations
    size_t div = 1;
    for (size_t i = 1; i < digits; i++) {
        div *= 10;
    }

    for (size_t i = 0; i < digits; i++) {
        char digit = '0' + (value / div % 10);
        array_list_push_assume_capacity(&sb->buffer, &digit);
        div /= 10;
    }

    return true;
}

MutSlice string_builder_to_string(StringBuilder* sb) {
    assert(sb);
    const char null_byte = '\0';
    if (!array_list_push(&sb->buffer, &null_byte) || !array_list_shrink_to_fit(&sb->buffer)) {
        return (MutSlice){
            .ptr    = NULL,
            .length = 0,
        };
    }

    return (MutSlice){
        .ptr    = sb->buffer.data,
        .length = sb->buffer.length - 1,
    };
}
