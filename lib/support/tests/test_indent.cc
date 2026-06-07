#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>

#include "indent.hh"

namespace ghoti::tests {

TEST_CASE("Indents over time") {
    Indent indent;

    SECTION("Empty indent") { CHECK(indent.current_branch() == ""); }

    SECTION("Single non-last") {
        const Indent::Guard g{indent, false};
        CHECK(indent.current_branch() == symbols::T_BRANCH);
    }

    SECTION("Single last") {
        const Indent::Guard g{indent, true};
        CHECK(indent.current_branch() == symbols::L_BRANCH);
    }

    SECTION("Nested levels") {
        const Indent::Guard g1{indent, false};
        {
            const Indent::Guard g2{indent, true};
            CHECK(indent.current_branch() ==
                  fmt::format("{}{}", symbols::VERT_BAR, symbols::L_BRANCH));
        }
    }

    SECTION("Nested levels") {
        const Indent::Guard g1{indent, true};
        const Indent::Guard g2{indent, true};
        const Indent::Guard g3{indent, false};
        CHECK(indent.current_branch() ==
              fmt::format("{}{}{}", symbols::EMPTY, symbols::EMPTY, symbols::T_BRANCH));
    }
}

} // namespace ghoti::tests
