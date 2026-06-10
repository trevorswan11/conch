#pragma once

#include <utility>

#include <diagnostic.hh>
#include <result.hh>
#include <types.hh>

namespace ghoti::mod {

enum class Error : u8 {
    PATH_DOES_NOT_EXIST,
    PATH_IS_NOT_FILE,
    FAILED_TO_OPEN_FILE,
    NORMALIZATION_FAILED,
    MODULE_PATH_NOT_RELATIVE,
    MODULE_DOES_NOT_EXIST,
    MODULE_ALREADY_EXISTS,
};

using Diagnostic = Diagnostic<Error>;

template <typename... Args>
[[nodiscard]] constexpr auto make_mod_err(Args&&... args) -> Err<Diagnostic> {
    return make_err<Diagnostic>(std::forward<Args>(args)...);
}

} // namespace ghoti::mod
