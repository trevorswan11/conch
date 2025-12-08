#pragma once

#include <stdbool.h>
#include <stdio.h>

#include "util/status.h"

typedef struct {
    FILE* in;
    FILE* out;
    FILE* err;
} FileIO;

FileIO file_io_std(void);

// Fills the io object, returning false if any arguments are NULL.
NODISCARD Status file_io_init(FileIO* io, FILE* in, FILE* out, FILE* err);

// Only call this if you want to close the internal files.
void file_io_deinit(FileIO* io);
