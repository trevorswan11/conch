#pragma once

#include "util/containers/array_list.h"
#include "util/io.h"
#include "util/status.h"

#define PROMPT ">>> "
#define CONTINUATION_PROMPT "... "
#define BUF_SIZE 4096
#define WELCOME_MESSAGE "Welcome to Conch REPL! Type 'exit' to quit."
#define EXIT_TOKEN "exit"

TRY_STATUS repl_start(void);
TRY_STATUS repl_run(FileIO* io, char* stream_buffer, ArrayList* stream_receiver);
TRY_STATUS repl_read_chunked(FileIO* io, char* stream_buffer, ArrayList* stream_receiver);
