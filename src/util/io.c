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

[[nodiscard]] Status file_io_init(FileIO* io, FILE* in, FILE* out, FILE* err) {
    assert(io && out && err);

    *io = (FileIO){
        .in  = in,
        .out = out,
        .err = err,
    };
    return SUCCESS;
}

void file_io_deinit(FileIO* io) {
    if (!io) { return; }

    if (io->in) { fclose(io->in); }
    fclose(io->out);
    fclose(io->err);
}
