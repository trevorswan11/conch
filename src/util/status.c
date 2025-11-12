#include <stdarg.h>
#include <stdio.h>

#include "util/status.h"

void debug_print(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fflush(stderr);
}

const char* status_name(Status status) {
    return STATUS_TYPE_NAMES[status];
}

void status_ignore(Status status) {
    MAYBE_UNUSED(status);
}
