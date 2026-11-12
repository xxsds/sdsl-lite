#include <algorithm>
#include <cmath>
#include <random>
#include <sstream>
#include <vector>

#include <sdsl/dac_vector_dp.hpp>
#include <sdsl/rrr_vector.hpp>

#include "gtest/gtest.h"

namespace
{

std::vector<std::vector<uint64_t>> inputs()
{
    std::mt19937_64 g(17);
    std::vector<std::vector<uint64_t>> result{{}, {0}, {~uint64_t(0)}, std::vector<uint64_t>(1000, 0)};
    std::vector<uint64_t> small, log_uniform, outliers;
    for (int i = 0; i < 20000; ++i)
    {
        small.push_back(1 + std::geometric_distribution<uint64_t>(0.5)(g));
        log_uniform.push_back(uint64_t(std::pow(2.0, std::uniform_real_distribution<>(0, 63)(g))));
        outliers.push_back(i % 1000 == 0 ? g() : g() % 4);
    }
    result.push_back(small);
    result.push_back(log_uniform);
    result.push_back(outliers);
    return result;
}

template <class t_dac>
void check(t_dac const & dac, std::vector<uint64_t> const & values)
{
    ASSERT_EQ(values.size(), dac.size());
    EXPECT_EQ(values.empty(), dac.empty());
    for (size_t i = 0; i < values.size(); ++i)
        ASSERT_EQ(values[i], dac[i]) << "i=" << i;
    EXPECT_TRUE(std::equal(dac.begin(), dac.end(), values.begin(), values.end()));
}

template <class t_dac>
void round_trip(int max_levels)
{
    for (auto const & values : inputs())
    {
        SCOPED_TRACE(::testing::Message() << "size=" << values.size() << " max_levels=" << max_levels);
        t_dac dac(values, max_levels);
        EXPECT_LE(dac.levels(), size_t(max_levels));
        check(dac, values);

        std::stringstream stream;
        dac.serialize(stream);
        t_dac loaded;
        loaded.load(stream);
        EXPECT_EQ(dac.levels(), loaded.levels());
        check(loaded, values);

        t_dac copy(loaded);
        t_dac moved(std::move(loaded));
        t_dac assigned;
        assigned = copy;
        check(copy, values);
        check(moved, values);
        check(assigned, values);
    }
}

TEST(dac_vector_dp_test, round_trip_with_level_limits)
{
    for (int max_levels : {1, 2, 3, 64})
        round_trip<sdsl::dac_vector_dp<>>(max_levels);
}

TEST(dac_vector_dp_test, round_trip_with_compressed_continuation_bits)
{
    round_trip<sdsl::dac_vector_dp<sdsl::rrr_vector<63>>>(64);
}

} // namespace

int main(int argc, char ** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
