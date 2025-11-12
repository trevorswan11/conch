#include <stdbool.h>
#include <stddef.h>

#include "util/containers/array_list.h"
#include "util/containers/string_builder.h"
#include "util/mem.h"
#include "util/status.h"

TRY_STATUS string_builder_init(StringBuilder* sb, size_t initial_length) {
    if (!sb) {
        return NULL_PARAMETER;
    } else if (initial_length == 0) {
        return EMPTY;
    }

    return array_list_init(&sb->buffer, initial_length, sizeof(char));
}

void string_builder_deinit(StringBuilder* sb) {
    if (!sb) {
        return;
    }
    array_list_deinit(&sb->buffer);
}

TRY_STATUS string_builder_append(StringBuilder* sb, char byte) {
    assert(sb);
    return array_list_push(&sb->buffer, &byte);
}

TRY_STATUS string_builder_append_many(StringBuilder* sb, const char* bytes, size_t length) {
    assert(sb);

    if (!bytes) {
        return NULL_PARAMETER;
    }

    PROPAGATE_IF_ERROR(array_list_ensure_total_capacity(&sb->buffer, sb->buffer.length + length));

    for (size_t i = 0; i < length; i++) {
        array_list_push_assume_capacity(&sb->buffer, &bytes[i]);
    }
    return SUCCESS;
}

TRY_STATUS string_builder_append_slice(StringBuilder* sb, Slice slice) {
    PROPAGATE_IF_ERROR(string_builder_append_many(sb, slice.ptr, slice.length));
    return SUCCESS;
}

TRY_STATUS string_builder_append_mut_slice(StringBuilder* sb, MutSlice slice) {
    PROPAGATE_IF_ERROR(string_builder_append_many(sb, slice.ptr, slice.length));
    return SUCCESS;
}

TRY_STATUS string_builder_append_size(StringBuilder* sb, size_t value) {
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

    PROPAGATE_IF_ERROR(array_list_ensure_total_capacity(&sb->buffer, sb->buffer.length + digits));

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

    return SUCCESS;
}

TRY_STATUS string_builder_to_string(StringBuilder* sb, MutSlice* slice) {
    assert(sb && slice);

    const char null_byte = '\0';
    PROPAGATE_IF_ERROR(array_list_push(&sb->buffer, &null_byte));
    PROPAGATE_IF_ERROR(array_list_shrink_to_fit(&sb->buffer));

    *slice = (MutSlice){
        .ptr    = sb->buffer.data,
        .length = sb->buffer.length - 1,
    };
    return SUCCESS;
}
