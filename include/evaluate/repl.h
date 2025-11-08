#pragma once

#include "util/containers/array_list.h"

#define PROMPT ">> "
#define BUF_SIZE 4096

void repl_start(void);
void repl_run(char* buf_in, ArrayList* buf_out);
