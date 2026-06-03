#pragma once

#include <config.h>

namespace ghoti::win32 {

// Enables UTF8 on creation, disables on destruction
//
// Can be safely created and destroyed multiple times on multiple threads
class RichConsole {
#if PLATFORM_WINDOWS
  public:
    RichConsole() noexcept;
    ~RichConsole();
#endif
};

} // namespace ghoti::win32
