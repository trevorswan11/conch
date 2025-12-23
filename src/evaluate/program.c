#include <stdio.h>

#include "evaluate/program.h"
#include "evaluate/repl.h"

#include "semantic/analyzer.h"

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

    SemanticAnalyzer seman;
    TRY_DO(seman_null_init(&seman, allocator), {
        lexer_deinit(&l);
        ast_deinit(&ast);
        parser_deinit(&p);
    });

    StringBuilder output_buffer;
    TRY_DO(string_builder_init(&output_buffer, BUF_SIZE), {
        lexer_deinit(&l);
        ast_deinit(&ast);
        parser_deinit(&p);
        seman_deinit(&seman);
    });

    *program = (Program){
        .lexer         = l,
        .parser        = p,
        .ast           = ast,
        .seman         = seman,
        .output_buffer = output_buffer,
        .io            = io,
        .allocator     = allocator,
    };
    return SUCCESS;
}

void program_deinit(Program* program) {
    if (!program) { return; }

    lexer_deinit(&program->lexer);
    parser_deinit(&program->parser);
    ast_deinit(&program->ast);
    seman_deinit(&program->seman);
    string_builder_deinit(&program->output_buffer);
}

NODISCARD Status program_run(Program* program, Slice input) {
    assert(program);
    if (input.length == 0) { return SUCCESS; }

    program->lexer.input        = input.ptr;
    program->lexer.input_length = input.length;
    TRY(lexer_consume(&program->lexer));

    TRY(parser_reset(&program->parser, &program->lexer));
    TRY(parser_consume(&program->parser, &program->ast));

    bool ok = true;
    if (program->parser.errors.length > 0) {
        TRY_IO(fprintf(program->io->err, "Parser errors:\n"));
        TRY(program_print_errors(&program->parser.errors, program->io));
        ok = false;
    } else {
        // Every iteration uses the same analyzer and global scope
        SemanticAnalyzer* seman = &program->seman;
        seman->ast              = &program->ast;
        TRY(seman_analyze(seman));
        if (seman->errors.length > 0) {
            TRY_IO(fprintf(program->io->err, "Semantic errors:\n"));
            TRY(program_print_errors(&seman->errors, program->io));
            ok = false;
        }
    }

    if (ok) {
        MutSlice output;
        TRY(ast_reconstruct(&program->ast, &program->output_buffer));
        TRY(string_builder_to_string(&program->output_buffer, &output));
        TRY_IO(fprintf(program->io->out, "%s", output.ptr));
    }
    TRY_IO(fprintf(program->io->out, "\n"));

    return SUCCESS;
}

NODISCARD Status program_print_errors(ArrayList* errors, FileIO* io) {
    assert(errors && errors->data && io);
    if (errors->length == 0) { return SUCCESS; }

    for (size_t i = 0; i < errors->length; i++) {
        MutSlice error;
        TRY(array_list_get(errors, i, &error));
        TRY_IO(fprintf(io->err, "\t%.*s", (int)error.length, error.ptr));

        if (i < errors->length - 1) { TRY_IO(fprintf(io->err, "\n")); }
    }

    return SUCCESS;
}
