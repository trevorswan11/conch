#include "catch_amalgamated.hpp"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

#include "file_io.hpp"
#include "fixtures.hpp"

extern "C" {
#include "ast/ast.h"
#include "evaluate/program.h"
#include "evaluate/repl.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "util/io.h"
#include "util/status.h"
}

TEST_CASE("Null program component deinit") {
    lexer_deinit(nullptr);
    parser_deinit(nullptr);
    ast_deinit(nullptr);
    program_deinit(nullptr);
    file_io_deinit(nullptr);
}

TEST_CASE("REPL with acceptable input") {
    const char* input_text = "var x: int = 123;\\\nconst y := 3;\nexit\n";

    TempFile temp_in("TMP_repl_in", input_text);
    TempFile temp_out("TMP_repl_out");
    TempFile temp_err("TMP_repl_err");

    FILE* in  = temp_in.open("rb+");
    FILE* out = temp_out.open("wb");
    FILE* err = temp_err.open("wb");

    FileIO                io = file_io_init(in, out, err);
    const Fixture<FileIO> fiof(io, file_io_deinit);

    char      buf[BUF_SIZE];
    ArrayList output;
    REQUIRE(STATUS_OK(array_list_init(&output, 1024, sizeof(char))));
    const Fixture<ArrayList> alf(output, array_list_deinit);

    REQUIRE(STATUS_OK(repl_run(&io, buf, &output)));

    std::ifstream     out_fs(temp_out.path(), std::ios::binary);
    const std::string captured_out((std::istreambuf_iterator<char>(out_fs)),
                                   std::istreambuf_iterator<char>());

    REQUIRE(captured_out.find(WELCOME_MESSAGE) != std::string::npos);
    REQUIRE(captured_out.find("var") != std::string::npos);
    REQUIRE(captured_out.find("const") != std::string::npos);
}

TEST_CASE("REPL with error input") {
    const char* input_text = "var x = 123;\nexit\n";

    TempFile temp_in("TMP_repl_in", input_text);
    TempFile temp_out("TMP_repl_out");
    TempFile temp_err("TMP_repl_err");

    FILE* in  = temp_in.open("rb+");
    FILE* out = temp_out.open("wb");
    FILE* err = temp_err.open("wb");

    FileIO io = file_io_init(in, out, err);
    const Fixture<FileIO> fiof(io, file_io_deinit);

    char      buf[BUF_SIZE];
    ArrayList output;
    REQUIRE(STATUS_OK(array_list_init(&output, 1024, sizeof(char))));
    const Fixture<ArrayList> alf(output, array_list_deinit);

    REQUIRE(STATUS_OK(repl_run(&io, buf, &output)));

    std::ifstream     out_fs(temp_out.path(), std::ios::binary);
    const std::string captured_out((std::istreambuf_iterator<char>(out_fs)),
                                   std::istreambuf_iterator<char>());

    std::ifstream     err_fs(temp_err.path(), std::ios::binary);
    const std::string captured_err((std::istreambuf_iterator<char>(err_fs)),
                                   std::istreambuf_iterator<char>());

    REQUIRE(captured_out.find(WELCOME_MESSAGE) != std::string::npos);
    REQUIRE(captured_err.find("Parser errors:") != std::string::npos);
}

TEST_CASE("REPL with incorrect buffer") {
    TempFile temp_in("TMP_repl_in", "2");
    TempFile temp_out("TMP_repl_out");
    TempFile temp_err("TMP_repl_err");

    FILE* in  = temp_in.open("rb+");
    FILE* out = temp_out.open("wb");
    FILE* err = temp_err.open("wb");

    FileIO                io = file_io_init(in, out, err);
    const Fixture<FileIO> fiof(io, file_io_deinit);

    char      buf[BUF_SIZE];
    ArrayList output;
    REQUIRE(STATUS_OK(array_list_init(&output, 1024, sizeof(size_t))));
    const Fixture<ArrayList> alf(output, array_list_deinit);

    REQUIRE(repl_run(&io, buf, &output) == Status::TYPE_MISMATCH);

    std::ifstream     err_fs(temp_err.path(), std::ios::binary);
    const std::string captured_err((std::istreambuf_iterator<char>(err_fs)),
                                   std::istreambuf_iterator<char>());
    REQUIRE(captured_err.find("ArrayList must be initialized for bytes") != std::string::npos);
}
