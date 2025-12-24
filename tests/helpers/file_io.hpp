#pragma once

#include <string>
#include <utility>

class TempFile {
  public:
    TempFile(std::string path, const std::string& content);

    explicit TempFile(std::string path) : filepath(std::move(path)) {}
    ~TempFile() { std::remove(filepath.c_str()); }

    FILE*       open(const char* permissions);
    std::string path() { return filepath; }

  private:
    std::string filepath;
};
