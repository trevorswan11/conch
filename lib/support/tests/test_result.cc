#include <string>
#include <string_view>

#include <catch2/catch_test_macros.hpp>

#include "option.hh"
#include "result.hh"
#include "types.hh"

namespace ghoti::tests {

TEST_CASE("Result traits") {
    STATIC_CHECK(traits::is_result<Result<i32, i64>>::value);
    STATIC_CHECK(traits::is_result<Result<i32&, std::string>>::value);
    STATIC_CHECK(traits::Result<Result<i32, i64>>);
    STATIC_CHECK_FALSE(traits::Result<i32>);
}

TEST_CASE("Try macro usage") {
    Result<i32, std::string_view> res;
    const auto                    unwrap = [&] -> opt::Option<Err<std::string_view>> {
        const auto val{TRY(res)};
        CHECK(val == 2);
        return opt::none;
    };

    res.emplace(2);
    CHECK_FALSE(unwrap());

    const std::string_view str{"Hello, World!"};
    res = make_err<std::string_view>(str);
    const auto err{unwrap()};
    REQUIRE(err);
    CHECK(err->error() == str);
}

} // namespace ghoti::tests
