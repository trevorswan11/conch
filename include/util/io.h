#pragma once

#include <stdbool.h>
#include <stdio.h>

typedef struct {
    FILE* in;
    FILE* out;
    FILE* err;
} FileIO;

// Fills the io object, returning false if any arguments are NULL.
bool file_io_init(FileIO* io, FILE* in, FILE* out, FILE* err);

// Only call this if you want to close the internal files.
void file_io_deinit(FileIO* io);
