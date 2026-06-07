#include "module/file_loader.hh"

#include <filesystem>
#include <fstream>
#include <ios>
#include <sstream>
#include <string>
#include <system_error>

#include <fmt/format.h>

#include "module/error.hh"

#include <result.hh>

namespace ghoti::mod {

auto FileLoader::load(const std::filesystem::path& path) -> Result<std::string, Diagnostic> {
    if (!std::filesystem::exists(path)) {
        return make_mod_err(fmt::format("Path '{}' does not exist", path.string()),
                            Error::PATH_DOES_NOT_EXIST);
    }

    if (!std::filesystem::is_regular_file(path)) {
        return make_mod_err(fmt::format("Path '{}' is not a file", path.string()),
                            Error::PATH_IS_NOT_FILE);
    }

    std::ifstream file{path, std::ios::in};
    if (!file.is_open()) {
        return make_mod_err(fmt::format("Failed to open file at path: '{}'", path.string()),
                            Error::FAILED_TO_OPEN_FILE);
    }
    std::stringstream buf;
    buf << file.rdbuf();
    return buf.str();
}

auto FileLoader::normalize(const std::filesystem::path& path)
    -> Result<std::filesystem::path, Error> {
    std::error_code ec;
    auto            canonical_path = std::filesystem::weakly_canonical(path, ec);
    if (ec) { return Err{Error::NORMALIZATION_FAILED}; }
    return canonical_path;
}

} // namespace ghoti::mod
