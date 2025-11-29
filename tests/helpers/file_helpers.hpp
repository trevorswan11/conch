#pragma once

#include <fstream>
#include <iostream>
#include <string>

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
