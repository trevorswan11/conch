#pragma once

#include "util/containers/array_list.h"
#include "util/error.h"
#include "util/io.h"

#define PROMPT ">> "
#define BUF_SIZE 4096
#define WELCOME_MESSAGE "Welcome to Conch REPL! Type 'exit' to quit."
#define EXIT_TOKEN "exit"

void     repl_start(void);
AnyError repl_run(FileIO* io, char* stream_buffer, ArrayList* stream_receiver);
AnyError repl_read_chunked(FileIO* io, char* stream_buffer, ArrayList* stream_receiver);
