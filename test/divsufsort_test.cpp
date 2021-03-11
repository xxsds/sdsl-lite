#include <algorithm>
#include <cstdint>

#include <sdsl/divsufsort.hpp>
#include <sdsl/int_vector.hpp>

#include <gtest/gtest.h>

namespace
{

using namespace sdsl;
using namespace std;

template <class T>
class divsufsort_test : public ::testing::Test
{};

using testing::Types;

std::vector<uint64_t> text_lengths{ 1, 10, 100, 1000, 10000, 100000, 100000 };
std::vector<uint64_t> alphabet_sizes{ 2, 4, 10, 100, 256 };

template <typename suffix_array_t, typename text_t>
inline void trivial_sa_construction(suffix_array_t & sa, text_t const & text)
{
    std::iota(sa.begin(), sa.end(), 0);
    std::sort(sa.begin(), sa.end(), [&text](uint64_t a, uint64_t b) -> bool {
        while (a < text.size() && b < text.size() && text[a] == text[b]) { ++a, ++b; }

        // If a == b == text.size(), cmp needs to return false.
        // Hence, we check b == text.size() before a == text.size().
        if (b == text.size()) return false;

        if (a == text.size()) return true;

        return text[a] < text[b];
    });
}

TEST(divsufsort_test, suffix_array_construction_32bit)
{
    std::mt19937_64 rng(std::chrono::system_clock::now().time_since_epoch().count());

    for (uint64_t const length : text_lengths)
    {
        for (uint64_t const alphabet_size : alphabet_sizes)
        {
            int_vector<32> sa1(length), sa2(length);
            int_vector<8> text(length);
            for (auto it = text.begin(); it != text.end(); ++it) *it = rng() % alphabet_size;

            divsufsort((const unsigned char *)text.data(), (int32_t *)sa1.data(), (int32_t)length);
            trivial_sa_construction(sa2, text);
            ASSERT_EQ(sa1, sa2);
        }
    }
}

TEST(divsufsort_test, suffix_array_construction_64bit)
{
    std::mt19937_64 rng(std::chrono::system_clock::now().time_since_epoch().count());

    for (uint64_t const length : text_lengths)
    {
        for (uint64_t const alphabet_size : alphabet_sizes)
        {
            int_vector<64> sa1(length), sa2(length);
            int_vector<8> text(length);
            for (auto it = text.begin(); it != text.end(); ++it) *it = rng() % alphabet_size;

            divsufsort((const unsigned char *)text.data(), (int64_t *)sa1.data(), (int64_t)length);
            trivial_sa_construction(sa2, text);
            ASSERT_EQ(sa1, sa2);
        }
    }
}

} // end namespace

int main(int argc, char * argv[])
{
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
