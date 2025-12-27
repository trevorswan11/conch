#include <stdarg.h>
#include <stdio.h>

#include "util/containers/array_list.h"
#include "util/containers/string_builder.h"
#include "util/status.h"

void debug_print([[maybe_unused]] const char* format, ...) {
#ifndef NDEBUG
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fflush(stderr);
#endif
}

const char* status_name(Status status) { return STATUS_TYPE_NAMES[status]; }

[[nodiscard]] Status put_status_error(ArrayList* errors, Status status, size_t line, size_t col) {
    assert(errors);
    assert(STATUS_ERR(status));

    Allocator* allocator = &errors->allocator;
    ASSERT_ALLOCATOR_PTR(allocator);

    StringBuilder builder;
    TRY(string_builder_init_allocator(&builder, 30, allocator));

    const char* status_literal = status_name(status);
    TRY_DO(string_builder_append_str_z(&builder, status_literal), string_builder_deinit(&builder));

    TRY_DO(error_append_ln_col(line, col, &builder), string_builder_deinit(&builder));

    MutSlice slice;
    TRY_DO(string_builder_to_string(&builder, &slice), string_builder_deinit(&builder));
    TRY_DO(array_list_push(errors, &slice), string_builder_deinit(&builder));
    return SUCCESS;
}

[[nodiscard]] Status error_append_ln_col(size_t line, size_t col, StringBuilder* sb) {
    assert(sb);
    const char line_no[] = " [Ln ";
    const char col_no[]  = ", Col ";

    TRY(string_builder_append_many(sb, line_no, sizeof(line_no) - 1));
    TRY(string_builder_append_unsigned(sb, (uint64_t)line));
    TRY(string_builder_append_many(sb, col_no, sizeof(col_no) - 1));
    TRY(string_builder_append_unsigned(sb, (uint64_t)col));
    TRY(string_builder_append(sb, ']'));

    return SUCCESS;
}
