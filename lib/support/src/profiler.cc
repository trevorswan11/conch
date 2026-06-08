#include "profiler.hh"

#ifdef GHOTI_PROFILE
#    include <chrono>
#    include <cstdint>
#    include <filesystem>
#    include <fstream>
#    include <memory>
#    include <mutex>
#    include <string_view>
#    include <thread>

#    include <fmt/format.h>
#    include <fmt/ostream.h>
#    include <fmt/std.h>

#    include "assert.hh"

namespace ghoti {

namespace chrono = std::chrono;

struct SessionDeleter {
    auto operator()(std::ofstream* ostream) -> void {
        if (!ostream) { return; }
        fmt::print(*ostream, "]}}");
        delete ostream;
    }
};

namespace {

constinit std::unique_ptr<std::ofstream, SessionDeleter> session;
constinit std::mutex                                     mutex;

auto write_scope(std::string_view     name,
                 micros<double>       start,
                 micros<std::int64_t> elapsed,
                 std::thread::id      tid) -> void {
    ASSERT(session && session->is_open(), "Writing cannot be done prior to initialization");

    std::scoped_lock lock{mutex};
    fmt::print(
        *session,
        R"(,{{"cat":"function","dur":{},"name":"{}","ph":"X","pid":0,"tid":"{}","ts":{:3f}}})",
        elapsed.count(),
        name,
        tid,
        start.count());
}

[[nodiscard]] constexpr auto to_int_micros(auto clock) -> auto {
    return chrono::time_point_cast<micros<std::int64_t>>(clock).time_since_epoch();
}

} // namespace

Profiler::Profiler(std::string_view path) {
    std::filesystem::path json{path};
    json.replace_filename(fmt::format("{}-profile.json", json.stem()));

    std::scoped_lock lock{mutex};
    session.reset(new std::ofstream{json});
    ASSERT(session->is_open(), "Profiler could not open output path");
    fmt::print(*session, R"({{"otherData": {{}},"traceEvents":[{{}})");
}

Profiler::~Profiler() {
    std::scoped_lock lock{mutex};
    session.reset();
}

Timer::Timer(const char* name) : name_{name}, start_{chrono::steady_clock::now()} {}

Timer::~Timer() {
    auto           end = to_int_micros(chrono::steady_clock::now());
    micros<double> high_res_start{start_.time_since_epoch()};
    auto           start   = to_int_micros(start_);
    auto           elapsed = end - start;
    write_scope(name_, high_res_start, elapsed, std::this_thread::get_id());
}

} // namespace ghoti
#endif
