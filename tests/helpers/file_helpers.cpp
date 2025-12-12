#include "file_helpers.hpp"

#include <fstream>
#include <iostream>

TempFile::TempFile(const std::string& path, const std::string& content) : filepath(path) {
    std::ofstream ofs(filepath, std::ios::binary);
    ofs.write(content.data(), content.size());
    ofs.close();
}

FILE* TempFile::open(const char* permissions) {
    FILE* f = fopen(filepath.c_str(), permissions);
    if (!f) {
        return NULL;
    }
    return f;
}
