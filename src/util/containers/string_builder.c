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
