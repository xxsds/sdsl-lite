#include <new>
#include <sstream>
#include <string>

#include <sdsl/bit_vectors.hpp>
#include <sdsl/wt_blcd.hpp>
#include <sdsl/wt_huff.hpp>
#include <sdsl/wt_hutu.hpp>

#include <gtest/gtest.h>

namespace
{
template <typename T>
class wt_empty_test : public ::testing::Test
{};

using implementations = ::testing::Types<sdsl::wt_blcd<>, sdsl::wt_huff<>, sdsl::wt_hutu<>>;
TYPED_TEST_SUITE(wt_empty_test, implementations, );

template <typename T>
std::string serialize_empty(unsigned char fill)
{
    // An empty tree's serialization must not depend on the previous contents of its storage.
    alignas(T) unsigned char storage[sizeof(T)];
    for (auto & byte : storage)
        static_cast<volatile unsigned char &>(byte) = fill;
    T * tree = new (storage) T;
    std::ostringstream out;
    tree->serialize(out);
    tree->~T();
    return out.str();
}

TYPED_TEST(wt_empty_test, deterministic_serialization)
{
    std::string const serialized = serialize_empty<TypeParam>(0xA5);
    ASSERT_EQ(serialized, serialize_empty<TypeParam>(0x5A));

    TypeParam restored;
    std::istringstream in(serialized);
    restored.load(static_cast<std::istream &>(in));
    ASSERT_TRUE(in.good());
    EXPECT_EQ(0u, restored.size());
    EXPECT_EQ(0u, restored.sigma);
    std::ostringstream out;
    restored.serialize(out);
    EXPECT_EQ(serialized, out.str());
}
} // namespace
