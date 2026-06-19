#include "module/memory_loader.hh"

#include <filesystem>
#include <string>
#include <utility>

#include <fmt/format.h>
#include <stdx/assert.hh>
#include <stdx/profiler.hh>
#include <stdx/result.hh>

#include "module/error.hh"

namespace ghoti::mod {

auto MemoryLoader::add(const std::filesystem::path& path, const std::string& content) -> void {
    PROFILE_FUNCTION();
    const auto normalized{normalize(path)};
    ASSERT(normalized);
    files_[*normalized] = content;
}

auto MemoryLoader::load(const std::filesystem::path& path)
    -> stdx::result<std::string, Diagnostic> {
    PROFILE_FUNCTION();
    auto normalized{normalize(path)};
    ASSERT(normalized);
    auto it{files_.find(normalized->string())};
    if (it == files_.end()) {
        return make_mod_err(
            fmt::format("Could not find path '{}' in virtual file system", path.string()),
            Error::PATH_DOES_NOT_EXIST);
    }
    return it->second;
}

} // namespace ghoti::mod
