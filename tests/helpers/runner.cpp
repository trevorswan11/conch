#include <catch_amalgamated.hpp>

#include <cstddef>
#include <new>

// NOLINTBEGIN
extern "C" auto launch(int argc, char** argv) -> int { return Catch::Session().run(argc, argv); }

extern "C" {
void* alloc(size_t size);
void  dealloc(void* ptr);
}

void* operator new(size_t __sz) {
    void* p = alloc(__sz);
    if (!p) { throw std::bad_alloc(); }
    return p;
}

void operator delete(void* p) noexcept { dealloc(p); }
void operator delete(void* p, size_t) noexcept { dealloc(p); }

auto operator new[](size_t __sz) -> void* { return operator new(__sz); }
void operator delete[](void* p) noexcept { operator delete(p); }
// NOLINTEND
