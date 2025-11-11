#include <stdbool.h>
#include <stdio.h>

#include "util/error.h"
#include "util/io.h"

FileIO file_io_std(void) {
    return (FileIO){
        .in  = stdin,
        .out = stdout,
        .err = stderr,
    };
}

AnyError file_io_init(FileIO* io, FILE* in, FILE* out, FILE* err) {
    if (!io || !in || !out || !err) {
        return NULL_PARAMETER;
    }

    *io = (FileIO){
        .in  = in,
        .out = out,
        .err = err,
    };
    return SUCCESS;
}

void file_io_deinit(FileIO* io) {
    fclose(io->in);
    fclose(io->out);
    fclose(io->err);
}
