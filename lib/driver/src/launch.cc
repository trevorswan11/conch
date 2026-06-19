#include "launch.hh"

#include <stdx/profiler.hh>
#include <stdx/result.hh>
#include <stdx/types.hh>

#include "clap/parser.hh"
#include "cmd/dispatcher.hh"

namespace ghoti::driver {

auto launch(i32 argc, char** argv) -> stdx::result<void, i32> {
    stdx::profiler profiler{argv[0]};
    clap::Parser   parser{argc, argv};
    TRY(parser.parse());

    cmd::Dispatcher dispatcher;
    return parser.get_parsed().visit(dispatcher).error_or(0);
}

} // namespace ghoti::driver
