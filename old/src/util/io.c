#include <assert.h>
#include <stdio.h>

#include "util/io.h"

FileIO file_io_std(void) {
    return (FileIO){
        .in  = stdin,
        .out = stdout,
        .err = stderr,
    };
}

FileIO file_io_init(FILE* in, FILE* out, FILE* err) {
    return (FileIO){
        .in  = in,
        .out = out,
        .err = err,
    };
}

void file_io_deinit(FileIO* io) {
    if (!io) { return; }

    if (io->in) { fclose(io->in); }
    fclose(io->out);
    fclose(io->err);
}
