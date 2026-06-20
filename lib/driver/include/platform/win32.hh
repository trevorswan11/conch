#pragma once

#include <ghoti/config.h> // IWYU pragma: export

namespace ghoti::win32 {

// Enables UTF8 on creation, disables on destruction
//
// Can be safely created and destroyed multiple times on multiple threads
class RichConsole {
#if GHOTI_WINDOWS
  public:
    RichConsole() noexcept;
    ~RichConsole();
#endif
};

} // namespace ghoti::win32
