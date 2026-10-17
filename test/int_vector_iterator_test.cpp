#include <algorithm>
#include <cstdint>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

#include <sdsl/int_vector.hpp>

#include <gtest/gtest.h>

namespace
{

template <uint8_t Width>
void check_iterators(uint8_t width)
{
    using vector_type = sdsl::int_vector<Width>;
    using iterator = typename vector_type::iterator;
    using const_iterator = typename vector_type::const_iterator;
    using difference_type = typename vector_type::difference_type;
    static_assert(std::is_convertible_v<iterator, const_iterator>);
    static_assert(!std::is_convertible_v<const_iterator, iterator>);
    static_assert(std::is_same_v<decltype(++std::declval<iterator &>()), iterator &>);
    static_assert(std::is_same_v<decltype(++std::declval<const_iterator &>()), const_iterator &>);
    static_assert(std::is_same_v<decltype(std::declval<iterator &>()++), iterator>);
    static_assert(std::is_same_v<decltype(std::declval<const_iterator &>()++), const_iterator>);

    static_assert(std::is_same_v<decltype(&iterator::operator+=), iterator & (iterator::*)(difference_type)>);
    static_assert(
        std::is_same_v<decltype(&const_iterator::operator+=), const_iterator & (const_iterator::*)(difference_type)>);
    EXPECT_EQ(iterator{}, iterator{});
    EXPECT_EQ(const_iterator{}, const_iterator{});
    vector_type values(193, 0, width);
    uint64_t mask = sdsl::bits::lo_set[width];
    for (size_t i = 0; i < values.size(); ++i)
        values[i] = (i * 73 + 11) & mask;

    auto exercise = [&](auto first, auto last)
    {
        using iter_type = decltype(first);
        EXPECT_EQ(last - first, difference_type(values.size()));
        EXPECT_EQ(first - last, -difference_type(values.size()));
        for (difference_type i = 0; i <= difference_type(values.size()); ++i)
        {
            auto it = first + i;
            EXPECT_EQ(it, i + first);
            EXPECT_EQ(it - first, i);
            EXPECT_EQ(last - it, difference_type(values.size()) - i);
            EXPECT_EQ(it == first, i == 0);
            EXPECT_EQ(it != first, i != 0);
            EXPECT_EQ(it < last, i < difference_type(values.size()));
            EXPECT_EQ(it > first, i > 0);
            EXPECT_TRUE(it >= first);
            EXPECT_TRUE(it <= last);
            iter_type copy(it), assigned;
            assigned = copy;
            EXPECT_EQ(assigned, it);
            if (i < difference_type(values.size()))
            {
                EXPECT_EQ(uint64_t(*it), uint64_t(values[i]));
                EXPECT_EQ(uint64_t(first[i]), uint64_t(values[i]));
                EXPECT_EQ(it++, copy);
                EXPECT_EQ(it, first + i + 1);
                EXPECT_EQ(--it, copy);
            }
            if (i > 0)
            {
                EXPECT_EQ(it--, copy);
                EXPECT_EQ(it, first + i - 1);
                EXPECT_EQ(++it, copy);
                EXPECT_EQ(uint64_t(copy[-1]), uint64_t(values[i - 1]));
            }
            for (difference_type target :
                 {difference_type(0), difference_type(63), difference_type(129), difference_type(values.size())})
            {
                auto moved = it;
                moved += target - i;
                EXPECT_EQ(moved, first + target);
                moved -= target - i;
                EXPECT_EQ(moved, it);
                EXPECT_EQ(it - (i - target), first + target);
            }
        }
    };
    exercise(values.begin(), values.end());
    exercise(values.cbegin(), values.cend());
    for (size_t i = 0; i < values.size(); ++i)
    {
        EXPECT_EQ(iterator(&values, i * width), values.begin() + i);
        EXPECT_EQ(const_iterator(&values, i * width), values.cbegin() + i);
        const_iterator converted(values.begin() + i);
        EXPECT_EQ(converted, values.cbegin() + i);
        auto writable = values.begin() + i;
        *writable = (i * 19) & mask;
        EXPECT_EQ(uint64_t(*converted), uint64_t(values[i]));
    }
    std::vector<uint64_t> expected(values.cbegin(), values.cend());
    std::reverse(expected.begin(), expected.end());
    std::reverse(values.begin(), values.end());
    EXPECT_TRUE(std::equal(expected.begin(), expected.end(), values.cbegin()));
    std::sort(expected.begin(), expected.end());
    std::sort(values.begin(), values.end());
    EXPECT_TRUE(std::equal(expected.begin(), expected.end(), values.cbegin()));
    for (uint64_t key : {uint64_t(0), uint64_t(1), uint64_t(17), mask})
        EXPECT_EQ(std::lower_bound(values.cbegin(), values.cend(), key) - values.cbegin(),
                  std::lower_bound(expected.begin(), expected.end(), key) - expected.begin());
    std::copy(expected.begin(), expected.end(), values.begin());
    EXPECT_TRUE(std::equal(expected.begin(), expected.end(), values.cbegin()));
}

TEST(IntVectorIterator, DynamicWidths)
{
    for (uint8_t width = 1; width <= 64; ++width)
    {
        SCOPED_TRACE(unsigned(width));
        check_iterators<0>(width);
    }
}

TEST(IntVectorIterator, FixedWidths)
{
    check_iterators<1>(1);
    check_iterators<3>(3);
    check_iterators<7>(7);
    check_iterators<31>(31);
    check_iterators<63>(63);
}

} // namespace
