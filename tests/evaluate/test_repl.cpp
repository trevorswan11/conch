#include "catch_amalgamated.hpp"

#include <stdio.h>

#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

#include "file_helpers.hpp"

extern "C" {
#include "evaluate/repl.h"
#include "util/io.h"
}

TEST_CASE("REPL with acceptable input") {
    const char* input_text = "var x: int = 123;\\\nconst y := 3;\nexit\n";

    TempFile temp_in("TMP_repl_in", input_text);
    TempFile temp_out("TMP_repl_out"), temp_err("TMP_repl_err");

    FILE* in  = temp_in.open("rb+");
    FILE* out = temp_out.open("wb");
    FILE* err = temp_err.open("wb");

    FileIO io;
    REQUIRE(STATUS_OK(file_io_init(&io, in, out, err)));

    char      buf[BUF_SIZE];
    ArrayList output;
    REQUIRE(STATUS_OK(array_list_init(&output, 1024, sizeof(char))));

    REQUIRE(STATUS_OK(repl_run(&io, buf, &output)));

    std::ifstream out_fs(temp_out.m_Path, std::ios::binary);
    std::string   captured_out((std::istreambuf_iterator<char>(out_fs)),
                             std::istreambuf_iterator<char>());

    std::ifstream err_fs(temp_err.m_Path, std::ios::binary);
    std::string   captured_err((std::istreambuf_iterator<char>(err_fs)),
                             std::istreambuf_iterator<char>());

    REQUIRE(captured_out.find(WELCOME_MESSAGE) != std::string::npos);
    REQUIRE(captured_out.find("var") != std::string::npos);
    REQUIRE(captured_out.find("const") != std::string::npos);

    array_list_deinit(&output);
    file_io_deinit(&io);
}

TEST_CASE("REPL with error input") {
    const char* input_text = "var x = 123;\nexit\n";

    TempFile temp_in("TMP_repl_in", input_text);
    TempFile temp_out("TMP_repl_out"), temp_err("TMP_repl_err");

    FILE* in  = temp_in.open("rb+");
    FILE* out = temp_out.open("wb");
    FILE* err = temp_err.open("wb");

    FileIO io;
    REQUIRE(STATUS_OK(file_io_init(&io, in, out, err)));

    char      buf[BUF_SIZE];
    ArrayList output;
    REQUIRE(STATUS_OK(array_list_init(&output, 1024, sizeof(char))));

    REQUIRE(STATUS_OK(repl_run(&io, buf, &output)));

    std::ifstream out_fs(temp_out.m_Path, std::ios::binary);
    std::string   captured_out((std::istreambuf_iterator<char>(out_fs)),
                             std::istreambuf_iterator<char>());

    std::ifstream err_fs(temp_err.m_Path, std::ios::binary);
    std::string   captured_err((std::istreambuf_iterator<char>(err_fs)),
                             std::istreambuf_iterator<char>());

    REQUIRE(captured_out.find(WELCOME_MESSAGE) != std::string::npos);
    REQUIRE(captured_err.find("Parser errors:") != std::string::npos);

    array_list_deinit(&output);
    file_io_deinit(&io);
}
