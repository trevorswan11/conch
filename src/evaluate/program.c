#include <stdio.h>

#include "evaluate/program.h"
#include "evaluate/repl.h"

NODISCARD Status program_init(Program* program, FileIO* io, Allocator allocator) {
    Lexer l;
    TRY(lexer_null_init(&l, allocator));

    AST ast;
    TRY_DO(ast_init(&ast, allocator), lexer_deinit(&l););

    Parser p;
    TRY_DO(parser_null_init(&p, io, allocator), {
        lexer_deinit(&l);
        ast_deinit(&ast);
    });

    StringBuilder output_buffer;
    TRY_DO(string_builder_init(&output_buffer, BUF_SIZE), {
        lexer_deinit(&l);
        ast_deinit(&ast);
        parser_deinit(&p);
    });

    *program = (Program){
        .lexer         = l,
        .parser        = p,
        .ast           = ast,
        .output_buffer = output_buffer,
        .io            = io,
        .allocator     = allocator,
    };
    return SUCCESS;
}

void program_deinit(Program* program) {
    if (!program) {
        return;
    }

    lexer_deinit(&program->lexer);
    parser_deinit(&program->parser);
    ast_deinit(&program->ast);
    string_builder_deinit(&program->output_buffer);
}

NODISCARD Status program_run(Program* program, Slice input) {
    assert(program);
    if (input.length == 0) {
        return SUCCESS;
    }

    program->lexer.input        = input.ptr;
    program->lexer.input_length = input.length;
    TRY(lexer_consume(&program->lexer));

    TRY(parser_reset(&program->parser, &program->lexer));
    TRY(parser_consume(&program->parser, &program->ast));

    if (program->parser.errors.length > 0) {
        TRY(program_print_parse_errors(program));
    }

    MutSlice output;
    TRY(ast_reconstruct(&program->ast, &program->output_buffer));
    TRY(string_builder_to_string(&program->output_buffer, &output));
    TRY_IO(fprintf(program->io->out, "%s\n", output.ptr));
    return SUCCESS;
}

NODISCARD Status program_print_parse_errors(Program* program) {
    assert(program && program->io && program->parser.errors.data);
    if (program->parser.errors.length == 0) {
        return SUCCESS;
    }

    FileIO*          io     = program->io;
    const ArrayList* errors = &program->parser.errors;

    TRY_IO(fprintf(io->err, "Parser errors:\n"));
    for (size_t i = 0; i < errors->length; i++) {
        MutSlice error;
        TRY(array_list_get(errors, i, &error));
        TRY_IO(fprintf(io->err, "\t%.*s", (int)error.length, error.ptr));

        if (i < errors->length - 1) {
            TRY_IO(fprintf(io->err, "\n"));
        }
    }

    return SUCCESS;
}
