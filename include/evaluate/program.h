#pragma once

#include "ast/ast.h"
#include "lexer/lexer.h"
#include "parser/parser.h"

#include "util/allocator.h"
#include "util/containers/string_builder.h"
#include "util/io.h"
#include "util/mem.h"
#include "util/status.h"

typedef struct {
    Lexer  lexer;
    Parser parser;
    AST    ast;

    StringBuilder output_buffer;
    FileIO*       io;
    Allocator     allocator;
} Program;

TRY_STATUS program_init(Program* program, FileIO* io, Allocator allocator);
void       program_deinit(Program* program);

TRY_STATUS program_run(Program* program, Slice input_line);
TRY_STATUS program_print_parse_errors(Program* program);
