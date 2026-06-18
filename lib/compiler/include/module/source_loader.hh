#pragma once

#include <filesystem>
#include <string>

#include <stdx/result.hh>

#include "module/error.hh"

namespace ghoti::mod {

class SourceLoader {
  public:
    virtual ~SourceLoader() = default;
    [[nodiscard]] virtual auto load(const std::filesystem::path& path)
        -> stdx::Result<std::string, Diagnostic> = 0;

    // Normalizes the path to behave as required by the loader
    [[nodiscard]] virtual auto normalize(const std::filesystem::path& path)
        -> stdx::Result<std::filesystem::path, Error> = 0;
};

} // namespace ghoti::mod
