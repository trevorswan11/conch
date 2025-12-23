#ifndef REPL_H
#define REPL_H

#include <stddef.h>

#include "util/containers/array_list.h"
#include "util/io.h"
#include "util/status.h"

#define PROMPT ">>> "
#define CONTINUATION_PROMPT "... "
static const size_t BUF_SIZE = 4096;
#define WELCOME_MESSAGE "Welcome to Conch REPL! Type 'exit' to quit."
#define EXIT_TOKEN "exit"

NODISCARD Status repl_start(void);
NODISCARD Status repl_run(FileIO* io, char* stream_buffer, ArrayList* stream_receiver);
NODISCARD Status repl_read_chunked(FileIO* io, char* stream_buffer, ArrayList* stream_receiver);

#endif
