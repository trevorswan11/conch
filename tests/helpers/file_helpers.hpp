#pragma once

#include <string>

class TempFile {
  public:
    TempFile(const std::string& path, const std::string& content);

    TempFile(const std::string& path) : filepath(path) {}
    ~TempFile() { std::remove(filepath.c_str()); }

    FILE*              open(const char* permissions);
    inline std::string path() { return filepath; }

  private:
    std::string filepath;
};
