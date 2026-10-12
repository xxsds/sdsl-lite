#include <array>
#include <random>
#include <string>
#include <type_traits>

#include <sdsl/hyb_vector.hpp>

#include <gtest/gtest.h>

namespace
{

template <uint32_t rate>
void check_select(sdsl::bit_vector const & bits)
{
    sdsl::hyb_vector<rate> vector(bits);
    typename sdsl::hyb_vector<rate>::select_0_type select0(&vector);
    typename sdsl::hyb_vector<rate>::select_1_type select1(&vector);
    typename sdsl::hyb_vector<rate>::rank_0_type rank0(&vector);
    typename sdsl::hyb_vector<rate>::rank_1_type rank1(&vector);
    uint64_t zeros = 0, ones = 0;
    for (uint64_t pos = 0; pos < bits.size(); ++pos)
    {
        ASSERT_EQ(bits[pos], vector[pos]);
        ASSERT_EQ(zeros, rank0(pos));
        ASSERT_EQ(ones, rank1(pos));
        if (bits[pos])
            ASSERT_EQ(pos, select1(++ones));
        else
            ASSERT_EQ(pos, select0(++zeros));
    }
    EXPECT_EQ(bits.size(), select0(zeros + 1));
    EXPECT_EQ(bits.size(), select1(ones + 1));
    EXPECT_EQ(zeros, rank0(bits.size()));
    EXPECT_EQ(ones, rank1(bits.size()));
}

template <typename T>
class hyb_select_test : public ::testing::Test
{};

using rates = ::testing::Types<std::integral_constant<uint32_t, 1>,
                               std::integral_constant<uint32_t, 2>,
                               std::integral_constant<uint32_t, 4>,
                               std::integral_constant<uint32_t, 16>,
                               std::integral_constant<uint32_t, 64>>;
TYPED_TEST_SUITE(hyb_select_test, rates, );

TYPED_TEST(hyb_select_test, block_encodings)
{
    // Uniform, two-run, minority, run-length and plain blocks, in both polarities.
    // Their different encoded lengths also leave plain blocks unaligned in the trunk.
    std::array<std::string, 13> patterns = {std::string(256, '0'),
                                            std::string(256, '1'),
                                            std::string(63, '0') + std::string(193, '1'),
                                            std::string(193, '1') + std::string(63, '0'),
                                            "0000000000000001",
                                            "1111111111111110",
                                            "0000000011111111",
                                            "1111111100000000",
                                            "000000000111111111",
                                            "111111111000000000",
                                            "01",
                                            // One-bit outer runs around a long implicit run.
                                            "0" + std::string(254, '1') + "0",
                                            "1" + std::string(254, '0') + "1"};
    sdsl::bit_vector bits(256 * patterns.size());
    for (uint64_t block = 0; block < patterns.size(); ++block)
        for (uint64_t i = 0; i < 256; ++i)
            bits[block * 256 + i] = patterns[block][i % patterns[block].size()] == '1';
    check_select<TypeParam::value>(bits);
}

TYPED_TEST(hyb_select_test, run_counts)
{
    // Both parities of the final two (implicit) runs, for every encoded run count.
    for (uint32_t runs = 3; runs <= 33; ++runs)
    {
        sdsl::bit_vector bits(256, 0);
        for (uint64_t pos = 0; pos < bits.size(); ++pos)
            bits[pos] = (pos * runs / bits.size()) & 1;
        check_select<TypeParam::value>(bits);
        bits.flip();
        check_select<TypeParam::value>(bits);
    }
}

TYPED_TEST(hyb_select_test, short_patterns)
{
    for (uint64_t size = 0; size <= 8; ++size)
        for (uint64_t pattern = 0; pattern < (uint64_t(1) << size); ++pattern)
        {
            sdsl::bit_vector bits(size, 0);
            for (uint64_t i = 0; i < size; ++i)
                bits[i] = (pattern >> i) & 1;
            check_select<TypeParam::value>(bits);
        }
}

TYPED_TEST(hyb_select_test, boundaries_and_densities)
{
    constexpr uint64_t superblock_size = 256 * TypeParam::value;
    std::mt19937_64 random(42);
    for (uint64_t size : std::array<uint64_t, 12>{0,
                                                  1,
                                                  63,
                                                  64,
                                                  65,
                                                  255,
                                                  256,
                                                  257,
                                                  superblock_size - 1,
                                                  superblock_size,
                                                  superblock_size + 1,
                                                  3 * superblock_size + 123})
    {
        for (unsigned density : {0, 1, 10, 50, 90, 99, 100})
        {
            SCOPED_TRACE(::testing::Message() << "size=" << size << ", density=" << density);
            sdsl::bit_vector bits(size);
            for (uint64_t i = 0; i < size; ++i)
                bits[i] = random() % 100 < density;
            check_select<TypeParam::value>(bits);
        }
    }
}

TYPED_TEST(hyb_select_test, empty_superblocks)
{
    constexpr uint64_t superblock_size = 256 * TypeParam::value;
    sdsl::bit_vector bits(5 * superblock_size, 0);
    // Repeated prefix counts must not move a select query past its occurrence.
    for (uint64_t pos : {superblock_size - 1, superblock_size, 4 * superblock_size})
        bits[pos] = 1;
    check_select<TypeParam::value>(bits);
    bits.flip();
    check_select<TypeParam::value>(bits);
}

} // namespace
