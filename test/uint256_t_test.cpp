#include <cstdint>
#include <type_traits>

#include <sdsl/uint256_t.hpp>

#include <gtest/gtest.h>

namespace
{

sdsl::uint256_t single_bit(int bit)
{
    if (bit < 64)
        return sdsl::uint256_t(uint64_t{1} << bit, 0, 0);
    if (bit < 128)
        return sdsl::uint256_t(0, uint64_t{1} << (bit - 64), 0);
    return sdsl::uint256_t(0, 0, sdsl::uint128_t{1} << (bit - 128));
}

TEST(uint256_t_test, shifts)
{
    for (int bit = 0; bit < 256; ++bit)
    {
        auto const value = single_bit(bit);
        for (int shift = 0; shift < 256; ++shift)
        {
            auto const left = bit + shift < 256 ? single_bit(bit + shift) : sdsl::uint256_t(0);
            auto const right = bit >= shift ? single_bit(bit - shift) : sdsl::uint256_t(0);
            ASSERT_EQ(left, value << shift) << "bit=" << bit << ", shift=" << shift;
            ASSERT_EQ(right, value >> shift) << "bit=" << bit << ", shift=" << shift;
        }
    }
}

TEST(uint256_t_test, const_operations)
{
    sdsl::uint256_t const a = single_bit(200) | 3;
    sdsl::uint256_t const b = single_bit(70);
    EXPECT_EQ(single_bit(200) + single_bit(70) + 3, a + b);
    EXPECT_EQ(single_bit(200) + 3 - single_bit(70), a - b);
    EXPECT_EQ(sdsl::uint256_t(3), a & ~single_bit(200));
    EXPECT_EQ(3, a.popcount());
    EXPECT_EQ(200, a.hi());
    EXPECT_EQ(200, a.select(3));
    EXPECT_EQ(3u, static_cast<uint64_t>(a));
}

TEST(uint256_t_test, builtin_operands)
{
    // The uint128_t fallback class is not trivially copyable, so uint256_t cannot be either.
    static_assert(!std::is_trivially_copyable_v<sdsl::uint128_t> || std::is_trivially_copyable_v<sdsl::uint256_t>);
    auto const low = (sdsl::uint128_t{1} << 100) | sdsl::uint128_t{7};
    sdsl::uint256_t const value = single_bit(201) | low;
    EXPECT_EQ(low, static_cast<sdsl::uint128_t>(value));
    EXPECT_EQ(sdsl::uint256_t(low), value & low);
    EXPECT_EQ(sdsl::uint256_t(8), (value & uint64_t{7}) + 1);
    EXPECT_EQ(value - 7, value & ~sdsl::uint256_t(7));
    EXPECT_TRUE(value > low);
    EXPECT_TRUE(low < value);
    EXPECT_TRUE(value != 0);
    EXPECT_TRUE(0 != value);
    EXPECT_FALSE(value == 0);
    EXPECT_TRUE(static_cast<bool>(single_bit(255)));
    EXPECT_FALSE(static_cast<bool>(sdsl::uint256_t{}));

    sdsl::uint256_t assigned;
    assigned = low;
    EXPECT_EQ(sdsl::uint256_t(low), assigned);
    // Negative values wrap modulo 2^256, as for built-in unsigned types.
    EXPECT_EQ(~sdsl::uint256_t{}, sdsl::uint256_t(-1));
}

TEST(uint256_t_test, compound_operators)
{
    sdsl::uint256_t value = single_bit(10) | single_bit(130);
    value <<= 100;
    EXPECT_EQ(single_bit(110) | single_bit(230), value);
    value >>= 110;
    EXPECT_EQ(single_bit(0) | single_bit(120), value);
    value &= single_bit(120);
    EXPECT_EQ(single_bit(120), value);
}

} // namespace
