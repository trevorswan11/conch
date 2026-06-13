#pragma once

#ifdef GHOTI_PROFILE
#    include <cassert>
#    include <chrono>
#    include <string_view>

#    include <fmt/ostream.h>
#    include <fmt/std.h>

#    include "utility.hh"

namespace ghoti {

struct Profiler {
    // The tracing json file is created next to the provided binary
    explicit Profiler(std::string_view binary_path);
    ~Profiler();

    Profiler(const Profiler&)                        = delete;
    auto operator=(const Profiler&) -> Profiler&     = delete;
    Profiler(Profiler&&) noexcept                    = delete;
    auto operator=(Profiler&&) noexcept -> Profiler& = delete;
};

class Timer {
  public:
    explicit Timer(const char* name);
    ~Timer();

    Timer(const Timer&)                        = delete;
    auto operator=(const Timer&) -> Timer&     = delete;
    Timer(Timer&&) noexcept                    = delete;
    auto operator=(Timer&&) noexcept -> Timer& = delete;

  private:
    const char*                                        name_;
    std::chrono::time_point<std::chrono::steady_clock> start_;
};

} // namespace ghoti

#    define PROFILE_SCOPE(name) \
        ::ghoti::Timer CONCAT(timer, __LINE__) { name }
#    define PROFILE_FUNCTION() PROFILE_SCOPE(__PRETTY_FUNCTION__)
#else
#    include <string_view>

#    define PROFILE_SCOPE(name)
#    define PROFILE_FUNCTION()

namespace ghoti {

// This is compiled out with argv[0]: https://godbolt.org/z/5jdK3ssor
struct Profiler {
    constexpr explicit Profiler(std::string_view) noexcept {}
};

// This exists purely as a test hook
struct Timer {
    constexpr explicit Timer(const char*) noexcept {}
};

} // namespace ghoti
#endif
