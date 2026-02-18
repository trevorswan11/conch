#include <cstddef>
#include <new>

#include <catch_amalgamated.hpp>

extern "C" {
auto launch(char* proc) -> int { return Catch::Session().run(1, &proc); }
auto alloc(size_t size) -> void*;
auto dealloc(void* ptr) -> void;
}

auto operator new(size_t size) -> void* {
    void* p = alloc(size);
    return p ? p : throw std::bad_alloc();
}

auto operator delete(void* p) noexcept -> void { dealloc(p); }
auto operator delete(void* p, size_t) noexcept -> void { dealloc(p); }

auto operator new[](size_t size) -> void* { return operator new(size); }
auto operator delete[](void* p) noexcept -> void { operator delete(p); }
