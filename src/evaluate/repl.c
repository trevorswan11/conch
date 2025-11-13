#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "evaluate/repl.h"

#include "lexer/lexer.h"

#include "util/allocator.h"
#include "util/containers/array_list.h"
#include "util/io.h"
#include "util/status.h"

TRY_STATUS repl_start(void) {
    char      buf_in[BUF_SIZE];
    ArrayList buf_out;
    PROPAGATE_IF_ERROR(array_list_init(&buf_out, 1024, sizeof(char)));

    FileIO io;
    PROPAGATE_IF_ERROR(file_io_init(&io, stdin, stdout, stderr));
    PROPAGATE_IF_ERROR(repl_run(&io, buf_in, &buf_out));

    array_list_deinit(&buf_out);
    return SUCCESS;
}

TRY_STATUS repl_run(FileIO* io, char* stream_buffer, ArrayList* stream_receiver) {
    assert(stream_buffer && stream_receiver);
    if (stream_receiver->item_size != sizeof(char)) {
        PROPAGATE_IF_IO_ERROR(fprintf(io->err, "ArrayList must be initialized for bytes\n"));
        return TYPE_MISMATCH;
    }

    PROPAGATE_IF_IO_ERROR(fprintf(io->out, WELCOME_MESSAGE));
    PROPAGATE_IF_IO_ERROR(fprintf(io->out, "\n"));
    Lexer l;
    PROPAGATE_IF_ERROR_DO(
        lexer_null_init(&l, standard_allocator),
        PROPAGATE_IF_IO_ERROR(fprintf(io->err, "Failed to default initialize lexer\n")));

    while (true) {
        array_list_clear_retaining_capacity(stream_receiver);
        PROPAGATE_IF_IO_ERROR(fprintf(io->out, PROMPT));
        PROPAGATE_IF_IO_ERROR(fflush(io->out));

        PROPAGATE_IF_ERROR_DO(repl_read_chunked(io, stream_buffer, stream_receiver),
                              lexer_deinit(&l));

        const char* line = (const char*)stream_receiver->data;
        if (strncmp(line, EXIT_TOKEN, sizeof(EXIT_TOKEN) - 1) == 0) {
            break;
        }

        l.input        = line;
        l.input_length = strlen(line);
        PROPAGATE_IF_ERROR(lexer_consume(&l));
        PROPAGATE_IF_ERROR(lexer_print_tokens(io, &l));
    }

    lexer_deinit(&l);
    return SUCCESS;
}

TRY_STATUS repl_read_chunked(FileIO* io, char* stream_buffer, ArrayList* stream_receiver) {
    const char null = 0;
    while (true) {
        char* result = fgets(stream_buffer, BUF_SIZE, io->in);
        if (!result) {
            PROPAGATE_IF_IO_ERROR(fprintf(io->err, "Failed to read from input stream.\n"));
            PROPAGATE_IF_IO_ERROR(fprintf(io->out, "\n"));
            return READ_ERROR;
        }

        const size_t n        = strlen(stream_buffer);
        const size_t required = stream_receiver->length + n;
        PROPAGATE_IF_ERROR(array_list_ensure_total_capacity(stream_receiver, required));

        memcpy((char*)stream_receiver->data + stream_receiver->length, stream_buffer, n);
        stream_receiver->length += n;

        if (stream_buffer[n - 1] == '\n' || feof(io->in)) {
            break;
        }
    }

    return array_list_push(stream_receiver, &null);
}
