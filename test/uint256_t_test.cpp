#include <cstdint>

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

} // namespace
