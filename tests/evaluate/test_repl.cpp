#include "catch_amalgamated.hpp"

#include <stdio.h>

#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

extern "C" {
#include "evaluate/repl.h"
#include "util/io.h"
}

struct TempFile {
    TempFile(const std::string& path, const std::string& content) : m_Path(path) {
        std::ofstream ofs(m_Path, std::ios::binary);
        ofs.write(content.data(), content.size());
        ofs.close();
    }

    TempFile(const std::string& path) : m_Path(path) {
    }

    ~TempFile() {
        std::remove(m_Path.c_str());
    }

    FILE* open(const char* permissions) const {
        FILE* f = fopen(m_Path.c_str(), permissions);
        if (!f) {
            return NULL;
        }
        return f;
    }

    std::string m_Path;
};

TEST_CASE("REPL with custom IO") {
    const char* input_text = "var x = 123;\nexit\n";

    TempFile temp_in("TMP_repl_in", input_text);
    TempFile temp_out("TMP_repl_out"), temp_err("TMP_repl_err");

    FILE* in  = temp_in.open("rb+");
    FILE* out = temp_out.open("wb");
    FILE* err = temp_err.open("wb");

    FileIO io;
    REQUIRE(file_io_init(&io, in, out, err));

    char      buf[BUF_SIZE];
    ArrayList output;
    array_list_init(&output, 1024, sizeof(uint8_t));

    repl_run(&io, buf, &output);

    std::ifstream out_fs(temp_out.m_Path, std::ios::binary);
    std::string   captured_out((std::istreambuf_iterator<char>(out_fs)),
                             std::istreambuf_iterator<char>());

    std::ifstream err_fs(temp_err.m_Path, std::ios::binary);
    std::string   captured_err((std::istreambuf_iterator<char>(err_fs)),
                             std::istreambuf_iterator<char>());

    REQUIRE(captured_out.find(WELCOME_MESSAGE) != std::string::npos);
    REQUIRE(captured_out.find("VAR") != std::string::npos);

    array_list_deinit(&output);
    file_io_deinit(&io);
}
