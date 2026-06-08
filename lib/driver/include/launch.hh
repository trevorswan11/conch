#pragma once

#include "result.hh"
#include "types.hh"

namespace ghoti::driver {

// Parses command line arguments and dispatches the input
auto launch(i32 argc, char** argv) -> Result<void, i32>;

} // namespace ghoti::driver
