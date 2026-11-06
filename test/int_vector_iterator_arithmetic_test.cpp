#include <algorithm>
#include <array>
#include <cstdint>
#include <iterator>
#include <random>
#include <vector>

#include <sdsl/int_vector.hpp>

#include <gtest/gtest.h>

namespace
{

template <uint8_t Width>
void check_iterators(uint8_t width)
{
    using vector_type = sdsl::int_vector<Width>;
    using difference_type = typename vector_type::difference_type;
    static_assert(std::random_access_iterator<typename vector_type::iterator>);
    static_assert(std::random_access_iterator<typename vector_type::const_iterator>);

    std::mt19937_64 random(419);
    uint64_t mask = ~uint64_t{} >> (64 - width);
    for (size_t size : {0, 1, 2, 63, 64, 65, 257})
    {
        vector_type values(size, 0, width);
        auto const & view = values;
        std::vector<uint64_t> expected(size);
        for (size_t i = 0; i < size; ++i)
        {
            expected[i] = random() & mask;
            values[i] = expected[i];
        }

        ASSERT_EQ(values.end() - values.begin(), difference_type(size));
        ASSERT_EQ(view.end() - view.begin(), difference_type(size));
        for (size_t origin : {size_t{0}, std::min(size_t{1}, size), size / 2, size})
        {
            auto mutable_first = values.begin() + origin;
            typename vector_type::const_iterator converted = mutable_first;
            ASSERT_EQ(converted, view.begin() + origin);
            for (size_t target = 0; target < size; ++target)
            {
                difference_type offset = difference_type(target) - difference_type(origin);
                ASSERT_EQ(converted[offset], expected[target]);
                ASSERT_EQ(uint64_t(mutable_first[offset]), expected[target]);
                ASSERT_EQ(*(converted + offset), expected[target]);

                auto moved = mutable_first;
                moved += offset;
                ASSERT_EQ(moved - mutable_first, offset);
                ASSERT_EQ(moved, values.begin() + target);
                moved -= offset;
                ASSERT_EQ(moved, mutable_first);

                expected[target] ^= mask;
                mutable_first[offset] = expected[target];
                ASSERT_EQ(view[target], expected[target]);
                if (target)
                {
                    ASSERT_EQ(view[target - 1], expected[target - 1]);
                }
                if (target + 1 < size)
                {
                    ASSERT_EQ(view[target + 1], expected[target + 1]);
                }
            }
        }

        auto first = view.begin(), last = view.end();
        for (size_t i = 0; i < size; ++i)
        {
            ASSERT_EQ(*first++, expected[i]);
            ASSERT_EQ(*--last, expected[size - i - 1]);
        }
        ASSERT_EQ(first, view.end());
        ASSERT_EQ(last, view.begin());

        std::sort(values.begin(), values.end());
        std::sort(expected.begin(), expected.end());
        for (size_t i = 0; i < size; ++i)
            ASSERT_EQ(view[i], expected[i]);
    }
}

TEST(int_vector_iterator_arithmetic, runtime_widths)
{
    for (uint8_t width = 1; width <= 64; ++width)
        check_iterators<0>(width);
}

TEST(int_vector_iterator_arithmetic, fixed_widths)
{
    check_iterators<1>(1);
    check_iterators<3>(3);
    check_iterators<7>(7);
    check_iterators<13>(13);
    check_iterators<31>(31);
    check_iterators<61>(61);
    check_iterators<63>(63);
    check_iterators<8>(8);
    check_iterators<16>(16);
    check_iterators<32>(32);
    check_iterators<64>(64);
}

template <uint8_t Width>
void check_bit_origins(uint8_t width)
{
    sdsl::int_vector<Width> values(257, 0, width);
    std::mt19937_64 random(419);
    for (size_t word = 0; word < (values.bit_size() + 63) / 64; ++word)
        values.data()[word] = random();

    for (ptrdiff_t origin = 0; origin < 128; ++origin)
    {
        typename sdsl::int_vector<Width>::const_iterator first(&values, origin);
        for (ptrdiff_t index = -2; index <= 2; ++index)
        {
            ptrdiff_t position = origin + index * width;
            if (position < 0 || uint64_t(position) + width > values.bit_size())
                continue;
            uint64_t expected = 0;
            for (uint8_t bit = 0; bit < width; ++bit)
                expected |= ((values.data()[(position + bit) / 64] >> ((position + bit) % 64)) & 1) << bit;
            ASSERT_EQ(first[index], expected);
        }
    }
}

TEST(int_vector_iterator_arithmetic, unaligned_bit_origins)
{
    for (uint8_t width = 1; width <= 64; ++width)
        check_bit_origins<0>(width);
    check_bit_origins<61>(61);
    check_bit_origins<63>(63);
}

TEST(int_vector_iterator_arithmetic, writes_preserve_surrounding_bits)
{
    std::mt19937_64 random(419);
    for (uint8_t width = 1; width <= 64; ++width)
    {
        for (uint8_t offset = 0; offset < 64; ++offset)
        {
            for (uint64_t value : {uint64_t{0}, ~uint64_t{0}, random()})
            {
                std::array<uint64_t, 4> words{random(), random(), random(), random()};
                auto expected = words;
                for (uint8_t bit = 0; bit < width; ++bit)
                {
                    size_t position = 64 + offset + bit;
                    uint64_t mask = uint64_t{1} << (position % 64);
                    expected[position / 64] =
                        (expected[position / 64] & ~mask) | (((value >> bit) & 1) << (position % 64));
                }
                sdsl::int_vector_reference<sdsl::int_vector<>> field(&words[1], offset, width);
                field = value;
                ASSERT_EQ(words, expected) << "width=" << unsigned(width) << ", offset=" << unsigned(offset);
            }
        }
    }
}

} // namespace
