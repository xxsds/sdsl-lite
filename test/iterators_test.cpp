#include <algorithm>
#include <iterator>
#include <ranges>

#include <sdsl/bit_vector_il.hpp>
#include <sdsl/hyb_vector.hpp>
#include <sdsl/iterators.hpp>
#include <sdsl/rrr_vector.hpp>
#include <sdsl/sd_vector.hpp>

#include <gtest/gtest.h>

namespace
{

TEST(iterators_test, lazy_range)
{
    sdsl::random_access_container values([](uint64_t i) { return 3 * i; }, 17);
    using iterator = decltype(values.begin());
    static_assert(std::random_access_iterator<iterator>);

    iterator first{}, second{};
    EXPECT_EQ(first, second);
    first = values.begin();
    EXPECT_EQ(0u, *first);
    EXPECT_EQ(48u, first[16]);
    EXPECT_EQ(17, std::ranges::distance(values));
    for (uint64_t query = 0; query < 55; ++query)
    {
        auto found = std::ranges::lower_bound(values, query);
        EXPECT_EQ(std::min<uint64_t>((query + 2) / 3, values.size()), found - values.begin());
    }
}

template <typename T>
class compressed_iterator_test : public ::testing::Test
{};

using compressed_vectors = ::testing::Types<sdsl::sd_vector<>,
                                            sdsl::rrr_vector<>,
                                            sdsl::bit_vector_il<>,
                                            sdsl::hyb_vector<>>;
TYPED_TEST_SUITE(compressed_iterator_test, compressed_vectors, );

TYPED_TEST(compressed_iterator_test, ranges)
{
    TypeParam empty;
    EXPECT_EQ(empty.begin(), empty.end());
    EXPECT_EQ(empty.end(), std::ranges::lower_bound(empty, 1));

    sdsl::bit_vector bits(1000, 0);
    for (uint64_t i = 500; i < bits.size(); ++i)
        bits[i] = 1;
    TypeParam values(bits);
    using iterator = decltype(values.begin());
    static_assert(std::random_access_iterator<iterator>);

    iterator first{}, second{};
    EXPECT_EQ(first, second);
    EXPECT_EQ(1000, std::ranges::distance(values));
    EXPECT_EQ(500, std::ranges::lower_bound(values, 1) - values.begin());
    EXPECT_EQ(values.end(), std::ranges::lower_bound(values, 2));
}

} // namespace
