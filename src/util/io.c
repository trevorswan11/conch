#include <stdbool.h>
#include <stdio.h>

#include "util/io.h"

bool file_io_init(FileIO* io, FILE* in, FILE* out, FILE* err) {
    if (!io || !in || !out || !err) {
        return false;
    }

    *io = (FileIO){
        .in  = in,
        .out = out,
        .err = err,
    };
    return true;
}

void file_io_deinit(FileIO* io) {
    fclose(io->in);
    fclose(io->out);
    fclose(io->err);
}
