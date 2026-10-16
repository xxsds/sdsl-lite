#include <algorithm>
#include <bit>
#include <random>
#include <vector>
#include <sdsl/bit_vector_il.hpp>
#include <gtest/gtest.h>
using namespace sdsl;
using ::testing::Types;

template <class T>
class cached_select_test : public ::testing::Test
{};

using CachedSelect = Types<select_support_il<0, 512>, select_support_il<1, 512>,
                           select_support_il<0, 4096>, select_support_il<1, 4096>>;
TYPED_TEST_SUITE(cached_select_test, CachedSelect, );

TYPED_TEST(cached_select_test, repeated_populations_and_partial_last_block)
{
    bit_vector bvec((1 << 23) + 17); // Large enough to populate the select cache.
    size_t words = (bvec.size() + 63) / 64;
    std::vector<uint64_t> prefixes(words + 1);
    std::mt19937_64 random(42);
    for (size_t word = 0; word < words; ++word)
    {
        uint64_t value = random();
        if (word % 193 < 64)
            value = 0;
        else if (word % 193 < 128)
            value = ~uint64_t{0};
        value &= bits::lo_set[std::min<uint64_t>(64, bvec.size() - 64 * word)];
        bvec.data()[word] = value;
        prefixes[word + 1] = prefixes[word] + std::popcount(value);
    }
    typename TypeParam::bit_vector_type vector(bvec);
    TypeParam select(&vector);
    uint64_t total = TypeParam::bit_pat ? prefixes.back() : bvec.size() - prefixes.back();
    for (size_t query = 0; query < 2000; ++query)
    {
        uint64_t occurrence = query == 0 ? 1 : query == 1 ? total : random() % total + 1;
        uint64_t position = select(occurrence);
        ASSERT_LT(position, bvec.size());
        ASSERT_EQ(TypeParam::bit_pat, bvec[position]);
        uint64_t ones = prefixes[position / 64]
                      + std::popcount(bvec.data()[position / 64] & bits::lo_set[position % 64]);
        ASSERT_EQ(occurrence - 1, TypeParam::bit_pat ? ones : position - ones);
    }
}
