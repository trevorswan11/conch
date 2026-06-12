#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <type_traits>

#include <catch2/catch_test_macros.hpp>

#include "helpers/inheritance.hh"
#include "option.hh"
#include "type_traits.hh"
#include "types.hh"

namespace ghoti::tests {

TEST_CASE("Ref construction checks") {
    STATIC_CHECK_FALSE(traits::Constructible<opt::detail::Ref<i32>, i32&&>);
    STATIC_CHECK(traits::TriviallyCopyable<opt::detail::Ref<i32>>);
}

TEST_CASE("Option template specialization") {
    STATIC_CHECK(std::is_same_v<opt::Option<i32&>, opt::detail::Ref<i32>>);
    STATIC_CHECK(std::is_same_v<opt::Option<const i32&>, opt::detail::Ref<const i32>>);
    STATIC_CHECK(std::is_same_v<opt::Option<i32>, std::optional<i32>>);
}

TEST_CASE("Option traits") {
    STATIC_CHECK(traits::is_option<opt::Option<i32>>::value);
    STATIC_CHECK(traits::is_option<opt::Option<i32&>>::value);
    STATIC_CHECK(traits::Option<opt::Option<i32>>);
    STATIC_CHECK(traits::Option<opt::Option<i32&>>);
    STATIC_CHECK_FALSE(traits::Option<i32>);
}

TEST_CASE("Ref basic construction") {
    i32                         val{42};
    const opt::detail::Ref<i32> opt{val};

    CHECK(opt.has_value());
    CHECK(static_cast<bool>(opt));
    CHECK(opt);
    CHECK(opt.value() == val);
    CHECK(*opt == 42);
    CHECK(&*opt == &val);
}

TEST_CASE("Ref null use & access") {
    const opt::detail::Ref<i32> opt1{};
    CHECK_FALSE(opt1.has_value());
    const opt::detail::Ref<i32> opt2{opt::none};
    CHECK_FALSE(opt2.has_value());
}

TEST_CASE("Ref conversions") {
    SECTION("Non-const -> const") {
        i32                               val{42};
        const opt::detail::Ref<i32>       mut_opt{val};
        const opt::detail::Ref<const i32> const_opt{mut_opt};
        CHECK(const_opt.has_value());
        CHECK(*const_opt == 42);
    }

    SECTION("Derived -> Base") {
        helpers::Derived                         d;
        const opt::detail::Ref<helpers::Derived> d_opt{d};
        const opt::detail::Ref<helpers::Base>    base_opt = d_opt;
        CHECK(base_opt.has_value());
        CHECK(base_opt->x == 10);
    }

    SECTION("To standard optional") {
        i32                         val{42};
        const opt::detail::Ref<i32> ref_opt{val};
        const auto                  std_opt{ref_opt.materialize()};
        CHECK(std_opt == 42);
    }
}

TEST_CASE("Ref reassignment") {
    i32                   a{1}, b{2};
    opt::detail::Ref<i32> opt{a};
    CHECK(*opt == 1);

    opt = b;
    CHECK(*opt == 2);

    opt = opt::none;
    CHECK_FALSE(opt.has_value());
}

TEST_CASE("Ref mutability") {
    i32                         val{42};
    const opt::detail::Ref<i32> opt{val};
    CHECK(*opt == 42);

    *opt = 1;
    CHECK(*opt == 1);
    CHECK(val == 1);
    CHECK(&*opt == &val);
}

TEST_CASE("Safe optional default equality") {
    i32                     x{10}, y{10}, z{20};
    const opt::Option<i32&> opt_x{x};
    const opt::Option<i32&> opt_y{y};
    const opt::Option<i32&> opt_z{z};
    const opt::Option<i32&> opt_null;

    CHECK(opt::safe_eq<i32&>(opt_x, opt_y));
    CHECK_FALSE(opt::safe_eq<i32&>(opt_x, opt_z));
    CHECK_FALSE(opt::safe_eq<i32&>(opt_x, opt_null));
    CHECK(opt::safe_eq<i32&>(opt_null, opt_null));
}

TEST_CASE("Safe optional custom equality") {
    std::string                     s1 = "APPLE", s2 = "apple";
    const opt::Option<std::string&> opt1{s1};
    const opt::Option<std::string&> opt2{s2};

    CHECK(opt::safe_eq<std::string&>(
        opt1, opt2, [](const std::string& a, const std::string& b) -> bool {
            return std::ranges::equal(a, b, [](char ac, char bc) -> bool {
                return std::tolower(ac) == std::tolower(bc);
            });
        }));
}

TEST_CASE("Ref transform on value") {
    i32               i{9};
    opt::Option<i32&> opt_i{i};

    const auto res{opt_i.transform([](const i32& i) -> i32 { return i + 2; })};
    REQUIRE(res);
    CHECK(*res == 11);
}

TEST_CASE("Ref transform on none") {
    opt::Option<i32&> opt_i{};

    const auto res{opt_i.transform([](const i32& i) -> i32 { return i + 2; })};
    CHECK_FALSE(res);
}

TEST_CASE("Boolean wrapper") {
    opt::Tribool b;
    CHECK_FALSE(b.has_value());
    CHECK_FALSE(b.value_or(false));
    CHECK_FALSE(b.value_or(0));

    b = true;
    CHECK(b.has_value());
    CHECK(b.value_or(false));
}

TEST_CASE("Boolean-std optional conversion") {
    std::optional<bool> std_b{true};
    opt::Tribool        my_b = std_b;
    REQUIRE(std_b.has_value());
    CHECK(*std_b == *my_b);

    std::optional<bool> std_extracted = my_b;
    REQUIRE(std_extracted.has_value());
    CHECK(*std_extracted);

    std::optional<bool> empty_std;
    my_b = empty_std;
    CHECK_FALSE(my_b.has_value());
}

TEST_CASE("Index wrapper") {
    opt::Size i;
    CHECK_FALSE(i.has_value());

    i = 0;
    CHECK(i.has_value());
    CHECK(*i == 0);
}

TEST_CASE("Index-std optional conversion") {
    std::optional<usize> std_i{42};
    opt::Size            my_i = std_i;
    REQUIRE(std_i.has_value());
    CHECK(*std_i == *my_i);

    std::optional<usize> std_extracted = my_i;
    REQUIRE(std_extracted.has_value());
    CHECK(*std_extracted == 42);

    std::optional<usize> empty_std;
    my_i = empty_std;
    CHECK_FALSE(my_i.has_value());
}

enum NonOptionableEnum : u8 {};

enum class OptionableEnum : u8 {
    A,
    B,
    C,
};

TEST_CASE("CompactOpt with enum") {
    STATIC_REQUIRE_FALSE(traits::Compactable<i32>);
    STATIC_REQUIRE_FALSE(traits::Compactable<NonOptionableEnum>);
    STATIC_REQUIRE(traits::Compactable<OptionableEnum>);
    STATIC_REQUIRE(sizeof(opt::Option<OptionableEnum>) == sizeof(OptionableEnum));
}

TEST_CASE("Optional enum wrapper") {
    opt::Option<OptionableEnum> e;
    CHECK_FALSE(e.has_value());

    e = OptionableEnum::A;
    CHECK(e.has_value());
    CHECK(*e == OptionableEnum::A);
}

TEST_CASE("Enum transform on value") {
    opt::Option<OptionableEnum> e{OptionableEnum::A};
    const auto res{e.transform([](const OptionableEnum&) -> auto { return OptionableEnum::B; })};
    REQUIRE(res);
    CHECK(*res == OptionableEnum::B);
}

TEST_CASE("Enum transform on none") {
    opt::Option<OptionableEnum> e{};
    const auto res{e.transform([](const OptionableEnum&) -> auto { return OptionableEnum::B; })};
    CHECK_FALSE(res);
}

TEST_CASE("Enum-std optional conversion") {
    std::optional<OptionableEnum> std_i{OptionableEnum::A};
    opt::Option<OptionableEnum>   my_i = std_i;
    REQUIRE(std_i.has_value());
    CHECK(*std_i == *my_i);

    std::optional<OptionableEnum> std_extracted = my_i;
    REQUIRE(std_extracted.has_value());
    CHECK(*std_extracted == OptionableEnum::A);

    std::optional<OptionableEnum> empty_std;
    my_i = empty_std;
    CHECK_FALSE(my_i.has_value());
}

} // namespace ghoti::tests
