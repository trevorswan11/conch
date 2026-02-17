#include <cstddef>
#include <new>

#include <catch_amalgamated.hpp>

extern "C" {
auto  launch(char* proc) -> int { return Catch::Session().run(1, &proc); }
void* alloc(size_t size);
void  dealloc(void* ptr);
}

void* operator new(size_t size) {
    void* p = alloc(size);
    return p ? p : throw std::bad_alloc();
}

void operator delete(void* p) noexcept { dealloc(p); }
void operator delete(void* p, size_t) noexcept { dealloc(p); }

auto operator new[](size_t size) -> void* { return operator new(size); }
void operator delete[](void* p) noexcept { operator delete(p); }
