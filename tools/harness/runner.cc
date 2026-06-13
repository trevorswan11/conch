#include <cstddef>
#include <new>

#include <catch2/catch_session.hpp>

#include <profiler.hh>
#include <types.hh>

using namespace ghoti;

// Catch2 shenanigans
extern "C" {
auto launch(i32 argc, char** argv) -> i32 {
    Profiler profiler{argv[0]};
    return Catch::Session().run(argc, argv);
}
}

#ifdef GHOTI_PROFILE
#    include <string>

#    include <catch2/catch_test_case_info.hpp>
#    include <catch2/catch_test_spec.hpp>
#    include <catch2/interfaces/catch_interfaces_reporter.hpp>
#    include <catch2/reporters/catch_reporter_event_listener.hpp>
#    include <catch2/reporters/catch_reporter_registrars.hpp>
#    include <fmt/format.h>

#    include <memory.hh>
namespace {

// https://github.com/catchorg/Catch2/blob/devel/docs/event-listeners.md
class TestTimerListener : public Catch::EventListenerBase {
  public:
    using EventListenerBase::EventListenerBase;

    auto testCaseStarting(const Catch::TestCaseInfo& info) -> void override {
        auto line_info{info.lineInfo};
        name_  = fmt::format(R"({}: "{}" (line {}))", line_info.file, info.name, line_info.line);
        timer_ = mem::make_nullable_box<Timer>(name_.c_str());
    }

    auto testCaseEnded(const Catch::TestCaseStats&) -> void override { timer_.reset(); }

  private:
    std::string             name_;
    mem::NullableBox<Timer> timer_;
};

} // namespace

CATCH_REGISTER_LISTENER(TestTimerListener)
#endif

// Allocator shenanigans
extern "C" {
auto alloc(usize size) -> void*;
auto dealloc(void* ptr) -> void;
}

auto operator new(usize size) -> void* {
    void* p = alloc(size);
    return p ? p : throw std::bad_alloc();
}

auto operator delete(void* p) noexcept -> void { dealloc(p); }
auto operator delete(void* p, usize) noexcept -> void { dealloc(p); }

auto operator new[](usize size) -> void* { return operator new(size); }
auto operator delete[](void* p) noexcept -> void { operator delete(p); }
