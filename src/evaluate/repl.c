#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "evaluate/repl.h"

#include "lexer/lexer.h"

#include "util/containers/array_list.h"
#include "util/io.h"

void repl_start(void) {
    char      buf_in[BUF_SIZE];
    ArrayList buf_out;
    array_list_init(&buf_out, 1024, sizeof(char));

    FileIO io;
    file_io_init(&io, stdin, stdout, stderr);
    repl_run(&io, buf_in, &buf_out);

    array_list_deinit(&buf_out);
}

void repl_run(FileIO* io, char* stream_buffer, ArrayList* stream_receiver) {
    assert(stream_buffer && stream_receiver);
    if (stream_receiver->item_size != sizeof(char)) {
        fprintf(io->err, "ArrayList must be initialized for bytes\n");
        return;
    }

    fprintf(io->out, WELCOME_MESSAGE);
    fprintf(io->out, "\n");
    Lexer l;
    if (!lexer_null_init(&l)) {
        fprintf(io->err, "Failed to default initialize lexer\n");
        return;
    }

    while (true) {
        array_list_clear_retaining_capacity(stream_receiver);
        fprintf(io->out, PROMPT);
        fflush(io->out);

        if (!repl_read_chunked(io, stream_buffer, stream_receiver)) {
            lexer_deinit(&l);
            return;
        }

        const char* line = (const char*)stream_receiver->data;
        if (strncmp(line, EXIT_TOKEN, sizeof(EXIT_TOKEN) - 1) == 0) {
            break;
        }

        l.input        = line;
        l.input_length = strlen(line);
        lexer_consume(&l);
        lexer_print_tokens(io, &l);
    }

    lexer_deinit(&l);
}

bool repl_read_chunked(FileIO* io, char* stream_buffer, ArrayList* stream_receiver) {
    const char null = 0;
    while (true) {
        char* result = fgets(stream_buffer, BUF_SIZE, io->in);
        if (!result) {
            fprintf(io->err, "Failed to read from input stream.\n");
            fprintf(io->out, "\n");
            return false;
        }

        const size_t n        = strlen(stream_buffer);
        const size_t required = stream_receiver->length + n;
        array_list_ensure_total_capacity(stream_receiver, required);

        memcpy((char*)stream_receiver->data + stream_receiver->length, stream_buffer, n);
        stream_receiver->length += n;

        if (stream_buffer[n - 1] == '\n' || feof(io->in)) {
            break;
        }
    }

    return array_list_push(stream_receiver, &null);
}
