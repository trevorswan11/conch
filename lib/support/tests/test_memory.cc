#include <memory>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "helpers/inheritance.hh"
#include "memory.hh"
#include "option.hh"
#include "type_traits.hh"
#include "types.hh"

namespace ghoti::tests {

using OkNullableBox     = mem::NullableBox<bool>;
using NotNullableBox    = bool;
using CustomNullableBox = mem::NullableBox<bool, void (*)(bool*)>;

TEST_CASE("Nullable box traits") {
    STATIC_CHECK(traits::is_nullable_box<std::unique_ptr<bool>>::value);
    STATIC_CHECK(traits::is_nullable_box_v<OkNullableBox>);
    STATIC_CHECK_FALSE(traits::is_nullable_box_v<NotNullableBox>);
    STATIC_CHECK(traits::is_nullable_box_v<CustomNullableBox>);
}

TEST_CASE("Safe nullable box default equality") {
    const mem::NullableBox<i32> opt_a{mem::make_nullable_box<i32>(100)};
    const mem::NullableBox<i32> opt_b{mem::make_nullable_box<i32>(100)};
    const mem::NullableBox<i32> opt_c{mem::make_nullable_box<i32>(200)};
    const mem::NullableBox<i32> opt_null;

    CHECK(mem::nullable_boxes_eq<i32>(opt_a, opt_b));
    CHECK_FALSE(mem::nullable_boxes_eq<i32>(opt_a, opt_c));
    CHECK_FALSE(mem::nullable_boxes_eq<i32>(opt_a, opt_null));
    CHECK(mem::nullable_boxes_eq<i32>(opt_null, opt_null));
}

TEST_CASE("Safe nullable box custom equality") {
    struct Node {
        usize            type_id;
        std::string_view name;
    };

    const mem::NullableBox<Node> a = mem::make_nullable_box<Node>(1, "foo");
    const mem::NullableBox<Node> b = mem::make_nullable_box<Node>(1, "bar");
    CHECK(mem::nullable_boxes_eq<Node>(
        a, b, [](const Node& an, const Node& bn) { return an.type_id == bn.type_id; }));
}

using OkBox     = mem::Box<bool>;
using NotBox    = bool;
using CustomBox = mem::Box<bool, void (*)(bool*)>;

TEST_CASE("Basic box construction") {
    auto b = mem::make_box<int>(42);
    CHECK(*b == 42);
}

TEST_CASE("Box invariant enforcement") {
    mem::NullableBox<int> n;
    CHECK_THROWS_AS(mem::Box<int>{std::move(n)}, mem::NullBoxException);
    int* p = nullptr;
    CHECK_THROWS_AS(mem::Box<int>{p}, mem::NullBoxException);
}

TEST_CASE("Box upcasting") {
    auto                    d = mem::make_box<helpers::Derived>();
    mem::Box<helpers::Base> b = std::move(d);
    CHECK(b->x == 10);
}

TEST_CASE("Box release") {
    auto b   = mem::make_box<int>(10);
    int* raw = b.release();
    CHECK(raw != nullptr);
    delete raw;
}

TEST_CASE("Box traits") {
    STATIC_CHECK(sizeof(mem::Box<int>) == 8);
    STATIC_CHECK_FALSE(traits::is_box<std::unique_ptr<bool>>::value);
    STATIC_CHECK(traits::is_box_v<OkBox>);
    STATIC_CHECK_FALSE(traits::is_box_v<NotBox>);
    STATIC_CHECK(traits::is_box_v<CustomBox>);
}

TEST_CASE("NonNull construction checks") {
    STATIC_CHECK_FALSE(std::is_constructible_v<mem::NonNull<i32>, i32&&>);
    STATIC_CHECK_FALSE(std::is_constructible_v<mem::NonNull<i32>, opt::None>);
    STATIC_CHECK(traits::TriviallyCopyable<mem::NonNull<i32>>);
}

TEST_CASE("NonNull basic usage") {
    i32                     val = 42;
    const mem::NonNull<i32> ptr{&val};

    CHECK(*ptr == 42);
    CHECK(ptr.get() == &val);
}

TEST_CASE("NonNull from optional reference") {
    i32                         val = 10;
    const opt::detail::Ref<i32> opt{val};

    const mem::NonNull<i32> ptr{opt};
    CHECK(*ptr == 10);

    const opt::detail::Ref<i32> empty;
    CHECK_THROWS_AS(mem::NonNull<i32>{empty}, std::bad_optional_access);
}

TEST_CASE("NonNull conversions") {
    helpers::Derived                     d;
    const mem::NonNull<helpers::Derived> d_ptr{&d};

    mem::NonNull<helpers::Base> b_ptr{d_ptr};
    CHECK(b_ptr->x == 10);
    mem::NonNull<const helpers::Derived> cd_ptr{d_ptr};
    CHECK(cd_ptr->y == 20);
}

} // namespace ghoti::tests
