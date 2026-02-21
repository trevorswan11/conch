#include <new>

#include <catch_amalgamated.hpp>

extern "C" {
auto launch(int argc, char** proc) -> int { return Catch::Session().run(argc, proc); }

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
