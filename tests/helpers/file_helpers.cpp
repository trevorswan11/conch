#include "file_helpers.hpp"

#include <fstream>
#include <iostream>

TempFile::TempFile(std::string path, const std::string& content) : filepath(std::move(path)) {
    std::ofstream ofs(filepath, std::ios::binary);
    ofs.write(content.data(), static_cast<std::streamsize>(content.size()));
    ofs.close();
}

FILE* TempFile::open(const char* permissions) { return fopen(filepath.c_str(), permissions); }
