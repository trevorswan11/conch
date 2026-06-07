#pragma once

#include <filesystem>
#include <string>

#include "module/error.hh"
#include "module/source_loader.hh"

#include <result.hh>

namespace ghoti::mod {

class FileLoader : public SourceLoader {
  public:
    // Attempts to obtain the file's source code from disk and load it into memory
    [[nodiscard]] auto load(const std::filesystem::path& path)
        -> Result<std::string, Diagnostic> override;

    // Converts the path to its weakly canonical representation
    [[nodiscard]] auto normalize(const std::filesystem::path& path)
        -> Result<std::filesystem::path, Error> override;
};

} // namespace ghoti::mod
