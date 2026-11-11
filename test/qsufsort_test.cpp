#include <algorithm>
#include <cstdint>
#include <numeric>
#include <random>
#include <vector>

#include <sdsl/qsufsort.hpp>

#include <gtest/gtest.h>

namespace
{

template <uint8_t Width>
void check_suffixes(std::vector<uint64_t> const & text)
{
    sdsl::int_vector<Width> sa;
    auto input = text;
    sdsl::qsufsort::construct_sa(sa, input);
    std::vector<uint64_t> expected(text.size());
    std::iota(expected.begin(), expected.end(), uint64_t{0});
    std::sort(expected.begin(),
              expected.end(),
              [&](auto a, auto b)
              {
                  return std::lexicographical_compare(text.begin() + a, text.end(), text.begin() + b, text.end());
              });
    ASSERT_EQ(sa.size(), expected.size());
    for (size_t i = 0; i < expected.size(); ++i)
        ASSERT_EQ(sa[i], expected[i]) << "suffix rank " << i;
}

TEST(qsufsort, iterator_boundaries)
{
    std::mt19937_64 random(419);
    for (size_t size : {1, 2, 6, 7, 8, 40, 41, 100, 257})
    {
        for (unsigned pattern = 0; pattern < 5; ++pattern)
        {
            std::vector<uint64_t> text(size + 1);
            for (size_t i = 0; i < size; ++i)
            {
                switch (pattern)
                {
                case 0:
                    text[i] = 97;
                    break;
                case 1:
                    text[i] = i % 2 + 1;
                    break;
                case 2:
                    text[i] = i + 1;
                    break;
                case 3:
                    text[i] = size - i;
                    break;
                default:
                    text[i] = random() % 1024 + 1;
                    break;
                }
            }
            check_suffixes<0>(text);
            check_suffixes<16>(text);
        }
    }
}

} // namespace
