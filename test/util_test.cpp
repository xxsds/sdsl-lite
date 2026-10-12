#include <sdsl/int_vector.hpp>
#include <sdsl/util.hpp>

#include <gtest/gtest.h>

namespace
{
struct base
{
    virtual ~base() = default;
};
struct derived : base
{};

TEST(class_name_test, caches_static_names_without_changing_them)
{
    sdsl::int_vector<8> first, second;
    EXPECT_EQ("int_vector", sdsl::util::class_name(first));
    EXPECT_EQ(sdsl::util::class_name(first), sdsl::util::class_name(second));
}

TEST(class_name_test, preserves_dynamic_type)
{
    base b;
    derived d;
    base const & reference = d;
    auto const base_name = sdsl::util::class_name(b);
    auto const derived_name = sdsl::util::class_name(d);
    EXPECT_NE(base_name, derived_name);
    EXPECT_EQ(derived_name, sdsl::util::class_name(reference));
    EXPECT_EQ(base_name, sdsl::util::class_name(b));
}
} // namespace
