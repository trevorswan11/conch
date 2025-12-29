#include <stdint.h>

#include "util/memory.h"

typedef struct ArenaBlock ArenaBlock;

typedef struct ArenaBlock {
    ArenaBlock* next;
    size_t      capacity;
    size_t      offset;
    uint8_t*    data;
} ArenaBlock;

typedef enum {
    DEFAULT,
    RETAIN_CAPACITY,
    ZERO_RETAIN_CAPACITY,
    FULL_RESET,
} ArenaResetMode;

// A dynamic arena allocator. The backing allocator is externally managed.
//
// When a new block is needed, the size scales by a power of two.
typedef struct {
    Allocator      child_allocator;
    ArenaBlock*    head;
    ArenaBlock*    current;
    ArenaResetMode reset_mode;
    size_t         previous_block_size;
} Arena;

const size_t ARENA_DEFAULT_SIZE = 256;

[[nodiscard]] Status arena_init(Allocator*     arena,
                                ArenaResetMode reset_mode,
                                size_t         initial_block_size,
                                Allocator*     child_allocator);
void                 arena_deinit(Allocator* arena);
void                 arena_reset(Allocator* arena);
