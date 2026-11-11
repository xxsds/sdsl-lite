// Queries in the last superblocks, for vectors that end at a superblock boundary and inside
// one. With asserts on, locate_block also checks that every descriptor load stays inside
// the current superblock's descriptors.
#include <random>

#include <sdsl/hyb_vector.hpp>

#include <gtest/gtest.h>

namespace
{

template <uint32_t rate>
void check_tail(uint64_t superblocks, uint64_t extra_bits, uint32_t seed)
{
    uint64_t const n = superblocks * rate * 256 + extra_bits;
    sdsl::bit_vector bits(n, 0);
    std::mt19937_64 rng(seed);
    // Non-uniform, varied blocks: mix dense random, sparse and run-like blocks.
    for (uint64_t pos = 0; pos < n; ++pos)
    {
        uint64_t block = pos / 256;
        switch (block % 3)
        {
            case 0: bits[pos] = rng() & 1; break;
            case 1: bits[pos] = (rng() % 37) == 0; break;
            default: bits[pos] = (pos / 19) & 1;
        }
    }
    sdsl::hyb_vector<rate> vector(bits);
    typename sdsl::hyb_vector<rate>::rank_1_type rank1(&vector);
    typename sdsl::hyb_vector<rate>::select_1_type select1(&vector);
    typename sdsl::hyb_vector<rate>::select_0_type select0(&vector);
    uint64_t const begin = n > 2 * rate * 256 ? n - 2 * rate * 256 : 0;
    uint64_t ones = 0;
    for (uint64_t pos = 0; pos < begin; ++pos)
        ones += bits[pos];
    for (uint64_t pos = begin; pos < n; ++pos)
    {
        ASSERT_EQ(bits[pos], vector[pos]) << pos;
        ASSERT_EQ(ones, rank1(pos)) << pos;
        if (bits[pos])
            ASSERT_EQ(pos, select1(++ones));
        else
            ASSERT_EQ(pos, select0(pos - ones + 1));
    }
    ASSERT_EQ(ones, rank1(n));
}

template <typename T>
class hyb_last_superblock_test : public ::testing::Test
{};

using rates = ::testing::Types<std::integral_constant<uint32_t, 1>,
                               std::integral_constant<uint32_t, 7>,
                               std::integral_constant<uint32_t, 9>,
                               std::integral_constant<uint32_t, 15>,
                               std::integral_constant<uint32_t, 17>,
                               std::integral_constant<uint32_t, 24>,
                               std::integral_constant<uint32_t, 100>,
                               std::integral_constant<uint32_t, 16>,
                               std::integral_constant<uint32_t, 64>>;
TYPED_TEST_SUITE(hyb_last_superblock_test, rates, );

TYPED_TEST(hyb_last_superblock_test, ends_at_superblock_boundary)
{
    for (uint64_t superblocks : {1, 2, 5})
        check_tail<TypeParam::value>(superblocks, 0, 7 + superblocks);
}

TYPED_TEST(hyb_last_superblock_test, ends_inside_superblock)
{
    for (uint64_t extra : {1, 255, 256, 257, 7 * 256 + 13})
        check_tail<TypeParam::value>(3, extra % (TypeParam::value * 256), 11 + extra);
}

} // namespace
