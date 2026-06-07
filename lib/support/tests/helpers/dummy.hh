#include <string>

#include "types.hh"

namespace ghoti::tests::helpers {

struct Foo {
    i32  value;
    auto operator==(const Foo&) const -> bool = default;
};

struct Bar {
    std::string value;
    auto        operator==(const Bar&) const -> bool = default;
};

struct Baz {
    f64  value;
    auto operator==(const Baz&) const -> bool = default;
};

} // namespace ghoti::tests::helpers
