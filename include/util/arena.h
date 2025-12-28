#include <stdint.h>

#include "util/memory.h"

typedef struct ArenaBlock ArenaBlock;

typedef struct ArenaBlock {
    ArenaBlock* next;
    size_t      capacity;
    size_t      offset;
    uint8_t*    data;
} ArenaBlock;

// A dynamic arena allocator.
//
// When a new block is needed, the size scales by a power of two.
typedef struct {
    Allocator   child_allocator;
    ArenaBlock* head;
    ArenaBlock* current;
    size_t      previous_block_size;
} Arena;

void*              arena_malloc(void* arena_ctx, size_t size);
void*              arena_calloc(void* arena_ctx, size_t count, size_t size);
void*              arena_realloc(void* arena_ctx, void* ptr, size_t size);
static inline void arena_free(NO_CTX, [[maybe_unused]] void* ptr) {}

[[nodiscard]] Status
     arena_init(Allocator* arena, size_t initial_block_size, Allocator* child_allocator);
void arena_deinit(Allocator* arena);
void arena_reset(Allocator* arena);
