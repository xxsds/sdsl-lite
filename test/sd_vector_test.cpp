#include <algorithm>
#include <sstream>

#include <sdsl/rrr_vector.hpp>
#include <sdsl/sd_vector.hpp>

#include <gtest/gtest.h>

using namespace sdsl;
using namespace std;

namespace
{

size_t const BV_SIZE = 1000000;

template <class T>
class sd_vector_test : public ::testing::Test
{};

using testing::Types;

typedef Types<sd_vector<>, sd_vector<rrr_vector<63>>> Implementations;

TYPED_TEST_SUITE(sd_vector_test, Implementations, );

TYPED_TEST(sd_vector_test, iterator_constructor)
{
    std::vector<uint64_t> pos;
    bit_vector bv(BV_SIZE);
    std::mt19937_64 rng;
    std::uniform_int_distribution<uint64_t> distribution(0, 9);
    auto dice = bind(distribution, rng);
    for (size_t i = 0; i < bv.size(); ++i)
    {
        if (0 == dice())
        {
            pos.emplace_back(i);
            bv[i] = 1;
        }
    }
    TypeParam sdv(pos.begin(), pos.end());
    for (size_t i = 0; i < bv.size(); ++i)
    {
        ASSERT_EQ((bool)sdv[i], (bool)bv[i]);
    }
}

TYPED_TEST(sd_vector_test, builder_constructor)
{
    std::vector<uint64_t> pos;
    bit_vector bv(BV_SIZE);
    std::mt19937_64 rng;
    std::uniform_int_distribution<uint64_t> distribution(0, 9);
    auto dice = bind(distribution, rng);
    size_t ones = 0;
    for (size_t i = 0; i < bv.size(); ++i)
    {
        if (0 == dice())
        {
            pos.emplace_back(i);
            bv[i] = 1;
            ones++;
        }
    }
    sd_vector_builder builder(BV_SIZE, ones);
    for (auto i : pos)
    {
        builder.set(i);
    }
    TypeParam sdv(builder);
    for (size_t i = 0; i < bv.size(); ++i)
    {
        ASSERT_EQ((bool)sdv[i], (bool)bv[i]);
    }
}

TYPED_TEST(sd_vector_test, builder_empty_constructor)
{
    sd_vector_builder builder(BV_SIZE, 0UL);
    TypeParam sdv(builder);
    for (size_t i = 0; i < BV_SIZE; ++i)
    {
        ASSERT_FALSE((bool)sdv[i]);
    }
}

TYPED_TEST(sd_vector_test, lookup_bucket_boundaries)
{
    for (uint64_t size : {uint64_t{1}, uint64_t{65}, uint64_t{4097}, (uint64_t{1} << 40) + 65})
        for (unsigned pattern = 0; pattern < 3; ++pattern)
        {
            SCOPED_TRACE(::testing::Message() << size << ":" << pattern);
            std::vector<uint64_t> positions;
            if (pattern == 1)
                positions = {size - 1};
            else if (pattern == 2)
            {
                for (uint64_t value : {uint64_t{0},
                                       uint64_t{1},
                                       uint64_t{4},
                                       uint64_t{5},
                                       uint64_t{6},
                                       uint64_t{63},
                                       uint64_t{64},
                                       uint64_t{65},
                                       size / 2,
                                       size - 1})
                    if (value < size)
                        positions.push_back(value);
                std::sort(positions.begin(), positions.end());
                positions.erase(std::unique(positions.begin(), positions.end()), positions.end());
            }
            sd_vector_builder builder(size, positions.size());
            for (auto value : positions)
                builder.set(value);
            TypeParam original(builder);
            std::stringstream bytes;
            original.serialize(bytes);
            TypeParam restored;
            restored.load(static_cast<std::istream &>(bytes));
            std::vector<uint64_t> queries;
            for (uint64_t i = 0; i < std::min(size, uint64_t{4097}); ++i)
                queries.push_back(i);
            for (auto value : positions)
                for (uint64_t distance = 0; distance < 65 && distance <= value; ++distance)
                    queries.push_back(value - distance);
            queries.push_back(size - 1);
            for (auto query : queries)
            {
                bool const expected = std::binary_search(positions.begin(), positions.end(), query);
                EXPECT_EQ(expected, original[query]);
                EXPECT_EQ(expected, restored[query]);
            }
        }
}

} // end namespace

int main(int argc, char * argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
