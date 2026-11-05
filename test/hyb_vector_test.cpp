#include <cstddef>
#include <random>

#include <sdsl/bit_vectors.hpp>
#include <sdsl/hyb_vector.hpp>

#include <gtest/gtest.h>

namespace
{
template <typename T>
class hyb_vector_test : public ::testing::Test
{};

// With an odd superblock rate, the superblock header size is not a multiple of 4.
using implementations = ::testing::Types<sdsl::hyb_vector<16>, sdsl::hyb_vector<15>, sdsl::hyb_vector<1>>;
TYPED_TEST_SUITE(hyb_vector_test, implementations, );

// Sparse blocks have short encodings, so the plain encoded random blocks after them start at unaligned offsets. This
// includes the last block, which is only partially filled. The zero blocks contain a uniform superblock for each rate.
sdsl::bit_vector mixed_bit_vector()
{
    std::mt19937_64 rng{};
    sdsl::bit_vector bv(97 * 256 + 200, 0);
    for (size_t i = 0; i < bv.size(); ++i)
    {
        size_t const block = i / 256;
        if (block >= 32 && block < 64)
            continue;
        bv[i] = block % 2 == 0 ? i % 100 == 7 : rng() & 1;
    }
    return bv;
}

TYPED_TEST(hyb_vector_test, access_rank_select)
{
    sdsl::bit_vector const bv = mixed_bit_vector();
    TypeParam const hyb(bv);
    typename TypeParam::rank_1_type const rank_1(&hyb);
    typename TypeParam::rank_0_type const rank_0(&hyb);
    typename TypeParam::select_1_type const select_1(&hyb);
    typename TypeParam::select_0_type const select_0(&hyb);

    size_t ones = 0;
    for (size_t i = 0; i < bv.size(); ++i)
    {
        ASSERT_EQ(bv[i], hyb[i]) << "i=" << i;
        ASSERT_EQ(ones, rank_1(i)) << "i=" << i;
        ASSERT_EQ(i - ones, rank_0(i)) << "i=" << i;
        if (bv[i])
        {
            ASSERT_EQ(i, select_1(++ones)) << "i=" << i;
        }
        else
        {
            ASSERT_EQ(i, select_0(i + 1 - ones)) << "i=" << i;
        }
    }
    ASSERT_EQ(ones, rank_1(bv.size()));
}

// A rate of 15 does not divide 2^23 blocks, so superblocks must not be assumed to end at bit 2^31.
TEST(hyb_vector_hblock_test, boundary)
{
    using hyb_type = sdsl::hyb_vector<15>;
    size_t const begin = (size_t{1} << 31) - 16 * 256;

    // The bits before begin stay zero, which only takes a moment to encode.
    sdsl::bit_vector bv((size_t{1} << 31) + 16 * 256, 0);
    std::mt19937_64 rng{};
    for (size_t i = begin; i < bv.size(); ++i)
        bv[i] = rng() & 1;

    hyb_type const hyb(bv);
    hyb_type::rank_1_type const rank_1(&hyb);

    size_t ones = 0;
    for (size_t i = begin; i < bv.size(); ++i)
    {
        ASSERT_EQ(bv[i], hyb[i]) << "i=" << i;
        ASSERT_EQ(ones, rank_1(i)) << "i=" << i;
        ones += bv[i];
    }
}
} // namespace
