#pragma once

#ifdef GHOTI_PROFILE
#    include <cassert>
#    include <chrono>
#    include <cstdint>
#    include <ratio>
#    include <string_view>
#    include <thread>

#    include <fmt/ostream.h>
#    include <fmt/std.h>

#    include "utility.hh"

namespace ghoti {

template <typename T> using micros = std::chrono::duration<T, std::micro>;

class Profiler {
  public:
    // The tracing json file is created next to the provided binary
    explicit Profiler(std::string_view binary_path);
    ~Profiler();

    Profiler(const Profiler&)                        = delete;
    auto operator=(const Profiler&) -> Profiler&     = delete;
    Profiler(Profiler&&) noexcept                    = delete;
    auto operator=(Profiler&&) noexcept -> Profiler& = delete;

    static auto write(std::string_view     name,
                      micros<double>       start,
                      micros<std::int64_t> elapsed,
                      std::thread::id      tid) -> void;
};

class Timer {
  public:
    explicit Timer(std::string_view name);
    ~Timer();

  private:
    std::string_view                                   name_;
    std::chrono::time_point<std::chrono::steady_clock> start_;
    bool                                               stopped_{false};
};

} // namespace ghoti

#    define PROFILE_SCOPE(name) \
        ::ghoti::Timer CONCAT(timer, __LINE__) { name }
#    define PROFILE_FUNCTION() PROFILE_SCOPE(__PRETTY_FUNCTION__)
#else
#    define PROFILE_SCOPE(name)
#    define PROFILE_FUNCTION()

namespace ghoti {

// This is compiled out with argv[0]: https://godbolt.org/z/45M5o8fj1
struct Profiler {
    constexpr explicit Profiler(auto) noexcept {}
};

} // namespace ghoti
#endif
