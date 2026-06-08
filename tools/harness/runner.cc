#include <cstddef>
#include <new>

#include <catch2/catch_session.hpp>
#include <fmt/format.h>
#include <fmt/std.h>

#include <profiler.hh>

extern "C" {
auto launch(int argc, char** argv) -> int {
    ghoti::Profiler profiler{argv[0]};
    return Catch::Session().run(argc, argv);
}

auto alloc(std::size_t size) -> void*;
auto dealloc(void* ptr) -> void;
}

auto operator new(std::size_t size) -> void* {
    void* p = alloc(size);
    return p ? p : throw std::bad_alloc();
}

auto operator delete(void* p) noexcept -> void { dealloc(p); }
auto operator delete(void* p, std::size_t) noexcept -> void { dealloc(p); }

auto operator new[](std::size_t size) -> void* { return operator new(size); }
auto operator delete[](void* p) noexcept -> void { operator delete(p); }
