#include "evaluate/repl.h"
#include "util/arena.h"

int main(void) {
    Allocator arena;
    TRY(arena_init(&arena, ZERO_RETAIN_CAPACITY, 256, &std_allocator));
    const Status error_code = repl_start(&std_allocator);
    arena_deinit(&arena);
    return error_code;
}
