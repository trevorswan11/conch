#include <stdio.h>

#include "evaluate/program.h"
#include "evaluate/repl.h"

TRY_STATUS program_init(Program* program, FileIO* io, Allocator allocator) {
    Lexer l;
    PROPAGATE_IF_ERROR(lexer_null_init(&l, allocator));

    AST ast;
    PROPAGATE_IF_ERROR_DO(ast_init(&ast, allocator), lexer_deinit(&l););

    Parser p;
    PROPAGATE_IF_ERROR_DO(parser_null_init(&p, io, allocator), {
        lexer_deinit(&l);
        ast_deinit(&ast);
    });

    StringBuilder output_buffer;
    PROPAGATE_IF_ERROR_DO(string_builder_init(&output_buffer, BUF_SIZE), {
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

TRY_STATUS program_run(Program* program, Slice input_line) {
    if (!program) {
        return NULL_PARAMETER;
    } else if (input_line.length == 0) {
        return SUCCESS;
    }

    program->lexer.input        = input_line.ptr;
    program->lexer.input_length = input_line.length;
    PROPAGATE_IF_ERROR(lexer_consume(&program->lexer));

    PROPAGATE_IF_ERROR(parser_reset(&program->parser, &program->lexer));
    PROPAGATE_IF_ERROR(parser_consume(&program->parser, &program->ast));

    if (program->parser.errors.length > 0) {
        PROPAGATE_IF_ERROR(program_print_parse_errors(program));
    }

    MutSlice output;
    PROPAGATE_IF_ERROR(ast_reconstruct(&program->ast, &program->output_buffer));
    PROPAGATE_IF_ERROR(string_builder_to_string(&program->output_buffer, &output));
    PROPAGATE_IF_IO_ERROR(fprintf(program->io->out, "%s\n", output.ptr));
    return SUCCESS;
}

TRY_STATUS program_print_parse_errors(Program* program) {
    if (!program || !program->io || !program->parser.errors.data) {
        return NULL_PARAMETER;
    } else if (program->parser.errors.length == 0) {
        return SUCCESS;
    }

    FileIO*          io     = program->io;
    const ArrayList* errors = &program->parser.errors;

    PROPAGATE_IF_IO_ERROR(fprintf(io->err, "Parser errors:\n"));
    for (size_t i = 0; i < errors->length; i++) {
        MutSlice error;
        PROPAGATE_IF_ERROR(array_list_get(errors, i, &error));
        PROPAGATE_IF_IO_ERROR(fprintf(io->err, "\t%.*s", (int)error.length, error.ptr));

        if (i < errors->length - 1) {
            PROPAGATE_IF_IO_ERROR(fprintf(io->err, "\n"));
        }
    }

    return SUCCESS;
}
