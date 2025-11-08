#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "evaluate/repl.h"
#include "lexer/lexer.h"
#include "util/containers/array_list.h"

void repl_start(void) {
    char      buf_in[BUF_SIZE];
    ArrayList buf_out;
    array_list_init(&buf_out, 1024, sizeof(uint8_t));

    printf("Welcome to Conch REPL! Type 'exit' to quit.\n");
    repl_run(buf_in, &buf_out);

    array_list_deinit(&buf_out);
}

void repl_run(char* stream_buffer, ArrayList* stream_receiver) {
    assert(stream_buffer && stream_receiver);
    if (stream_receiver->item_size != sizeof(uint8_t)) {
        fprintf(stderr, "ArrayList must be initialized for bytes\n");
        return;
    }

    Lexer*        l    = lexer_create("NULL");
    const uint8_t null = 0;

    while (true) {
        // Refresh our input and print the prompt
        array_list_clear_retaining_capacity(stream_receiver);
        printf(PROMPT);
        fflush(stdout);

        // Read the line, possibly in multiple chunks
        while (true) {
            char* result = fgets(stream_buffer, BUF_SIZE, stdin);
            if (!result) {
                printf("\n");
                lexer_destroy(l);
                return;
            }

            const size_t n        = strlen(stream_buffer);
            const size_t required = stream_receiver->length + n;
            array_list_ensure_total_capacity(stream_receiver, required);

            memcpy((uint8_t*)stream_receiver->data + stream_receiver->length, stream_buffer, n);
            stream_receiver->length += n;

            if (stream_buffer[n - 1] == '\n' || feof(stdin)) {
                break;
            }
        }
        array_list_push(stream_receiver, &null);

        // Now we can use buf_out freely
        const char* line = (const char*)stream_receiver->data;
        if (strcmp(line, "exit") == 0) {
            break;
        }

        l->input        = line;
        l->input_length = strlen(line);
        lexer_consume(l);
        lexer_print_tokens(l);
    }

    lexer_destroy(l);
}
