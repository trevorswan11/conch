#include "profiler.hh"

#ifdef GHOTI_PROFILE
#    include <chrono>
#    include <cstdint>
#    include <filesystem>
#    include <fstream>
#    include <iomanip>
#    include <ios>
#    include <memory>
#    include <mutex>
#    include <ostream>
#    include <sstream>
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

std::unique_ptr<std::ofstream, SessionDeleter> session;
std::mutex                                     mutex;

auto write_scope(std::string_view     name,
                 micros<double>       start,
                 micros<std::int64_t> elapsed,
                 std::thread::id      tid) -> void {
    ASSERT(session, "Writing cannot be done prior to initialization");
    std::stringstream json;
    json << std::setprecision(3) << std::fixed;

    fmt::print(json, ",{{");
    fmt::print(json, R"("cat":"function",)");
    fmt::print(json, R"("dur":{},)", elapsed.count());
    fmt::print(json, R"("name":"{}",)", name);
    fmt::print(json, R"("ph":"X","pid":0,"tid":"{}",)", tid);
    fmt::print(json, R"("ts":{}}})", start.count());

    std::scoped_lock lock{mutex};
    fmt::print(*session, "{}", json.view());
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

Timer::Timer(std::string_view name) : name_{name}, start_{chrono::steady_clock::now()} {}

Timer::~Timer() {
    if (stopped_) { return; }
    auto           end = to_int_micros(chrono::steady_clock::now());
    micros<double> high_res_start{start_.time_since_epoch()};
    auto           start   = to_int_micros(start_);
    auto           elapsed = end - start;

    write_scope(name_, high_res_start, elapsed, std::this_thread::get_id());
    stopped_ = true;
}

} // namespace ghoti
#endif
