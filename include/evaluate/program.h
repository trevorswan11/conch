#pragma once

#include "ast/ast.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "semantic/analyzer.h"

#include "util/allocator.h"
#include "util/containers/string_builder.h"
#include "util/mem.h"
#include "util/status.h"

typedef struct FileIO    FileIO;
typedef struct ArrayList ArrayList;

typedef struct {
    Lexer            lexer;
    Parser           parser;
    AST              ast;
    SemanticAnalyzer seman;

    StringBuilder output_buffer;
    FileIO*       io;
    Allocator     allocator;
} Program;

NODISCARD Status program_init(Program* program, FileIO* io, Allocator allocator);
void             program_deinit(Program* program);

NODISCARD Status program_run(Program* program, Slice input);
NODISCARD Status program_print_errors(ArrayList* errors, FileIO* io);
