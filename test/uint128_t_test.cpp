#include <cstdint>
#include <type_traits>
#include <utility>

#include <sdsl/bits.hpp>

#include <gtest/gtest.h>

// Exercise the fallback on GCC/Clang too; restore the macro after the headers.
#pragma push_macro("__GNUC__")
#undef __GNUC__
#include <sdsl/uint256_t.hpp>
#pragma pop_macro("__GNUC__")

namespace
{

template <typename T>
T single_bit(int bit)
{
    auto const word = uint64_t{1} << (bit % 64);
    if constexpr (std::is_same_v<T, sdsl::uint128_t>)
        return T(bit < 64 ? word : 0, bit >= 64 ? word : 0);
    else
        return T(bit < 64 ? word : 0,
                 bit >= 64 && bit < 128 ? word : 0,
                 sdsl::uint128_t(bit >= 128 && bit < 192 ? word : 0, bit >= 192 ? word : 0));
}

template <typename T>
class fallback_uint_test : public ::testing::Test
{};

using Implementations = ::testing::Types<sdsl::uint128_t, sdsl::uint256_t>;
TYPED_TEST_SUITE(fallback_uint_test, Implementations, );

TYPED_TEST(fallback_uint_test, shifts)
{
    int const width = std::is_same_v<TypeParam, sdsl::uint128_t> ? 128 : 256;
    for (int bit = 0; bit < width; ++bit)
    {
        auto const value = single_bit<TypeParam>(bit);
        for (int shift = 0; shift < width; ++shift)
        {
            auto const left = bit + shift < width ? single_bit<TypeParam>(bit + shift) : TypeParam(0);
            auto const right = bit >= shift ? single_bit<TypeParam>(bit - shift) : TypeParam(0);
            ASSERT_EQ(left, value << shift) << "bit=" << bit << ", shift=" << shift;
            ASSERT_EQ(right, value >> shift) << "bit=" << bit << ", shift=" << shift;
        }
    }
}

} // namespace
