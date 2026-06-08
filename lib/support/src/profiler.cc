#include "profiler.hh"

#ifdef GHOTI_PROFILE
#    include <chrono>
#    include <cstdint>
#    include <filesystem>
#    include <fstream>
#    include <iterator>
#    include <memory>
#    include <mutex>
#    include <string_view>
#    include <thread>

#    include <fmt/base.h>
#    include <fmt/format.h>
#    include <fmt/ostream.h>
#    include <fmt/std.h>

#    include "assert.hh"
#    include "fixed/vector.hh"
#    include "types.hh"

namespace ghoti {

namespace chrono = std::chrono;

namespace {

struct SessionDeleter {
    auto operator()(std::ofstream* ostream) -> void {
        if (!ostream) { return; }
        fmt::print(*ostream, "]}}");
        delete ostream;
    }
};

class Buffer;

constinit std::unique_ptr<std::ofstream, SessionDeleter> session;
constinit std::mutex                                     mutex;
constinit fixed::Vector<Buffer*, 1'024>                  buffers;

class Buffer {
  public:
    static constexpr usize BUF_SIZE{32UZ * 1'024UZ};
    static constexpr usize HEADROOM{512UZ};

  public:
    constexpr Buffer() = default;
    ~Buffer()          = default;

    Buffer(const Buffer&)                        = delete;
    auto operator=(const Buffer&) -> Buffer&     = delete;
    Buffer(Buffer&&) noexcept                    = delete;
    auto operator=(Buffer&&) noexcept -> Buffer& = delete;

    [[nodiscard]] auto back_inserter() noexcept -> auto { return std::back_inserter(buf_); }

    auto ensure_capacity() -> void {
        if (buf_.size() + HEADROOM >= BUF_SIZE) {
            std::scoped_lock lock{mutex};
            flush();
        }
    }

    // Must be called with the global mutex held
    auto flush() -> void {
        if (buf_.empty() || !session || !session->is_open()) { return; }
        fmt::print(*session, "{}", std::string_view{buf_.data(), buf_.size()});
        buf_.clear();
    }

  private:
    fixed::Vector<char, BUF_SIZE> buf_;
};

struct BufferManager {
    Buffer data;

    BufferManager() {
        std::scoped_lock lock{mutex};

        // Reuse empty slots to prevent buffer overflows
        bool inserted = false;
        for (auto*& buf : buffers) {
            if (!buf) {
                buf      = &data;
                inserted = true;
                break;
            }
        }
        if (!inserted) { buffers.emplace_back(&data); }
    }

    ~BufferManager() {
        std::scoped_lock lock{mutex};
        data.flush();

        // Cannot use a std algorithm due to fixed::Vector non-conformance
        for (auto*& buf : buffers) {
            if (buf == &data) {
                buf = nullptr;
                break;
            }
        }
    }

    BufferManager(const BufferManager&)                        = delete;
    auto operator=(const BufferManager&) -> BufferManager&     = delete;
    BufferManager(BufferManager&&) noexcept                    = delete;
    auto operator=(BufferManager&&) noexcept -> BufferManager& = delete;
};

auto write_scope(std::string_view     name,
                 micros<double>       start,
                 micros<std::int64_t> elapsed,
                 std::thread::id      tid) -> void {
    ASSERT(session && session->is_open(), "Writing cannot be done prior to initialization");

    thread_local BufferManager manager;
    manager.data.ensure_capacity();

    fmt::format_to(
        manager.data.back_inserter(),
        R"(,{{"cat":"function","dur":{},"name":"{}","ph":"X","pid":0,"tid":"{}","ts":{:.3f}}})",
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
    for (auto* buf : buffers) {
        if (buf) { buf->flush(); }
    }
    buffers.clear();
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
