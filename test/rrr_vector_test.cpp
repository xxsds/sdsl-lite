#include <cstdint>
#include <random>
#include <type_traits>

#include <sdsl/rrr_vector.hpp>

#include <gtest/gtest.h>

namespace
{

// Bit vectors with runs of zeros, runs of ones and random parts of the given density,
// so that blocks of all types and both polarities of the superblocks (m_invert) occur.
sdsl::bit_vector make_bits(uint64_t size, double density, uint64_t seed)
{
    std::mt19937_64 rng(seed);
    std::bernoulli_distribution coin(density);
    sdsl::bit_vector bits(size, 0);
    for (uint64_t start = 0; start < size; start += 700)
    {
        uint64_t kind = rng() % 4;
        for (uint64_t pos = start; pos < std::min(size, start + 700); ++pos)
            bits[pos] = kind == 0 ? 0 : (kind == 1 ? 1 : coin(rng));
    }
    return bits;
}

template <uint16_t t_bs, uint16_t t_k>
void check_queries(sdsl::bit_vector const & bits)
{
    using rrr_type = sdsl::rrr_vector<t_bs, sdsl::int_vector<>, t_k>;
    rrr_type vector(bits);
    typename rrr_type::rank_1_type rank1(&vector);
    typename rrr_type::rank_0_type rank0(&vector);
    typename rrr_type::select_1_type select1(&vector);
    typename rrr_type::select_0_type select0(&vector);
    uint64_t ones = 0;
    for (uint64_t pos = 0; pos < bits.size(); ++pos)
    {
        bool bit = bits[pos];
        ASSERT_EQ(bit, vector[pos]) << pos;
        ASSERT_EQ(ones, rank1(pos)) << pos;
        ASSERT_EQ(pos - ones, rank0(pos)) << pos;
        auto inverse = vector.inverse_select(pos);
        ASSERT_EQ(bit ? ones : pos - ones, inverse.first) << pos;
        ASSERT_EQ(bit, inverse.second) << pos;
        ASSERT_EQ(bit ? ones + 1 : 0, vector.conditional_rank(pos)) << pos;
        if (bit)
            ASSERT_EQ(pos, select1(++ones)) << pos;
        else
            ASSERT_EQ(pos, select0(pos + 1 - ones)) << pos;
    }
    EXPECT_EQ(ones, rank1(bits.size()));
    for (uint64_t pos = 0; pos < bits.size(); pos += 13)
    {
        uint8_t len = std::min<uint64_t>(64, bits.size() - pos);
        ASSERT_EQ(bits.get_int(pos, len), vector.get_int(pos, len)) << pos;
    }
}

template <typename T>
class rrr_query_test : public ::testing::Test
{};

template <uint16_t t_bs, uint16_t t_k>
using params = std::integer_sequence<uint16_t, t_bs, t_k>;

using block_sizes = ::testing::Types<params<3, 32>,
                                     params<15, 32>,
                                     params<31, 4>,
                                     params<63, 32>,
                                     params<64, 8>,
                                     params<100, 4>,
                                     params<127, 32>,
                                     params<128, 2>,
                                     params<129, 8>,
                                     params<255, 32>,
                                     params<256, 4>>;
TYPED_TEST_SUITE(rrr_query_test, block_sizes, );

template <uint16_t t_bs, uint16_t t_k>
void check_all(std::integer_sequence<uint16_t, t_bs, t_k>)
{
    uint64_t seed = 0;
    for (double density : {0.0, 0.01, 0.1, 0.3, 0.5, 0.7, 0.9, 0.99, 1.0})
        for (uint64_t size : {uint64_t(0), uint64_t(1), uint64_t(t_bs), uint64_t(20000), uint64_t(20000 + t_bs / 2)})
            check_queries<t_bs, t_k>(make_bits(size, density, ++seed));
}

TYPED_TEST(rrr_query_test, matches_bit_vector)
{
    check_all(TypeParam{});
}

} // namespace
