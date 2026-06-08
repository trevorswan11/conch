#include "launch.hh"

#include "clap/parser.hh"
#include "cmd/dispatcher.hh"

#include <profiler.hh>
#include <result.hh>
#include <types.hh>

namespace ghoti::driver {

auto launch(i32 argc, char** argv) -> Result<void, i32> {
    Profiler     profiler{argv[0]};
    clap::Parser parser{argc, argv};
    TRY(parser.parse());

    cmd::Dispatcher dispatcher;
    return parser.get_parsed().visit(dispatcher).error_or(0);
}

} // namespace ghoti::driver
