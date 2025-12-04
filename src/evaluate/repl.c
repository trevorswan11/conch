#include <assert.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "evaluate/program.h"
#include "evaluate/repl.h"

#include "util/allocator.h"
#include "util/containers/string_builder.h"
#include "util/status.h"

static volatile sig_atomic_t interrupted = 0;

static inline void sigint_handler(int sig) {
    MAYBE_UNUSED(sig);
    interrupted = 1;
}

TRY_STATUS repl_start(void) {
    signal(SIGINT, sigint_handler);

    char      buf_in[BUF_SIZE];
    ArrayList buf_out;
    PROPAGATE_IF_ERROR(
        array_list_init_allocator(&buf_out, BUF_SIZE, sizeof(char), standard_allocator));

    FileIO io = file_io_std();
    PROPAGATE_IF_ERROR_DO(repl_run(&io, buf_in, &buf_out), array_list_deinit(&buf_out));

    array_list_deinit(&buf_out);
    return SUCCESS;
}

TRY_STATUS repl_run(FileIO* io, char* stream_buffer, ArrayList* stream_receiver) {
    assert(stream_buffer && stream_receiver);
    if (stream_receiver->item_size != sizeof(char)) {
        PROPAGATE_IF_IO_ERROR(fprintf(io->err, "ArrayList must be initialized for bytes\n"));
        return TYPE_MISMATCH;
    }

    Program program;
    PROPAGATE_IF_ERROR(program_init(&program, io, standard_allocator));

    PROPAGATE_IF_IO_ERROR(fprintf(io->out, WELCOME_MESSAGE));
    PROPAGATE_IF_IO_ERROR(fprintf(io->out, "\n"));

    while (true) {
        // Interrupts will cancel current input but not the process
        if (interrupted) {
            interrupted = 0;
            fprintf(io->out, "\n");
            array_list_clear_retaining_capacity(stream_receiver);
            array_list_clear_retaining_capacity(&program.output_buffer.buffer);
            continue;
        }

        array_list_clear_retaining_capacity(stream_receiver);
        PROPAGATE_IF_IO_ERROR_DO(fprintf(io->out, PROMPT), program_deinit(&program));
        PROPAGATE_IF_IO_ERROR_DO(fflush(io->out), program_deinit(&program));
        PROPAGATE_IF_IO_ERROR_DO(fflush(io->err), program_deinit(&program));

        PROPAGATE_IF_ERROR_DO(repl_read_chunked(io, stream_buffer, stream_receiver),
                              program_deinit(&program));

        const char*  line        = (const char*)stream_receiver->data;
        const size_t line_length = strlen(line);
        if (strncmp(line, EXIT_TOKEN, sizeof(EXIT_TOKEN) - 1) == 0) {
            break;
        } else if (line_length == 1 && strncmp(line, "\n", 1) == 0) {
            continue;
        } else if (line_length == 2 && strncmp(line, "\r\n", 2) == 0) {
            continue;
        }

        PROPAGATE_IF_ERROR_DO(program_run(&program, slice_from_str_s(line, line_length)),
                              program_deinit(&program));
    }

    program_deinit(&program);
    return SUCCESS;
}

TRY_STATUS repl_read_chunked(FileIO* io, char* stream_buffer, ArrayList* stream_receiver) {
    const char null = 0;
    while (true) {
        if (!fgets(stream_buffer, BUF_SIZE, io->in)) {
            PROPAGATE_IF_IO_ERROR(fprintf(io->err, "Failed to read from input stream.\n"));
            PROPAGATE_IF_IO_ERROR(fprintf(io->out, "\n"));
            return READ_ERROR;
        }

        // Trim any platform line endings from the end of the buffer
        // A chunk may not contain a line ending, indicating it exceeds BUF SIZE
        size_t n = strlen(stream_buffer);
        bool   chunk_includes_newline =
            (n > 0 && (stream_buffer[n - 1] == '\n' || stream_buffer[n - 1] == '\r'));
        while (n > 0 && (stream_buffer[n - 1] == '\n' || stream_buffer[n - 1] == '\r')) {
            stream_buffer[n - 1] = '\0';
            n -= 1;
        }

        // We only need to do this appendage if there was something real read
        if (n > 0) {
            size_t needed = stream_receiver->length + n;
            PROPAGATE_IF_ERROR(array_list_ensure_total_capacity(stream_receiver, needed));
            memcpy((char*)stream_receiver->data + stream_receiver->length, stream_buffer, n);
            stream_receiver->length += n;
        }

        if (n == 0) {
            break;
        } else if (!chunk_includes_newline) {
            continue;
        }

        // A line consisting only of double backslash means that its a multiline continuation
        if (n == 2 && stream_buffer[0] == '\\' && stream_buffer[1] == '\\') {
            break;
        }

        // At this point, we saw a newline and have to check for line continuation
        if (n >= 1 && stream_buffer[n - 1] == '\\') {
            ((char*)stream_receiver->data)[stream_receiver->length - 1] = '\n';

            fprintf(io->out, CONTINUATION_PROMPT);
            fflush(io->out);

            continue;
        }

        break;
    }

    // The inner loop should handle the null terminator, this is defensive
    return array_list_push(stream_receiver, &null);
}
