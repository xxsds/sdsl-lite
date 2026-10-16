#include <array>
#include <cstring>
#include <span>
#include <sdsl/bit_vector_helper.hpp>
#include <gtest/gtest.h>

TEST(word_rank_test, byte_and_word_storage)
{
    std::array<uint64_t, 4> words = {0, uint64_t{1} << 63, 0x5555555555555555ULL, ~uint64_t{0}};
    for (size_t offset = 0; offset < alignof(uint64_t); ++offset)
    {
        SCOPED_TRACE(offset);
        std::array<uint8_t, sizeof(words) + alignof(uint64_t) - 1> bytes;
        std::memcpy(bytes.data() + offset, words.data(), sizeof(words));
        auto data = std::span<uint8_t const>(bytes.data() + offset, sizeof(words));
        uint64_t ones = 0;
        for (uint64_t position = 0; position < 256; ++position)
        {
            ASSERT_EQ(ones, sdsl::detail::rank_in_words(std::span(words), position));
            ASSERT_EQ(ones, sdsl::detail::rank_in_words(data, position));
            ones += (words[position / 64] >> (position % 64)) & 1;
        }
    }
}

TEST(word_rank_test, long_rank_prefixes)
{
    std::array<uint64_t, 17> words;
    for (size_t word = 0; word < words.size(); ++word)
        words[word] = 0x0123456789abcdefULL * (word + 1);
    constexpr uint64_t initial_rank = uint64_t{1} << 40;
    uint64_t expected = initial_rank;
    for (size_t prefix = 0; prefix <= 64 * words.size(); ++prefix)
    {
        auto data = std::span<uint64_t const>(words).first((prefix + 63) / 64);
        ASSERT_EQ(expected, sdsl::detail::rank_in_words(data, prefix, initial_rank));
        if (prefix < 64 * words.size())
            expected += (words[prefix / 64] >> (prefix % 64)) & 1;
    }
    ASSERT_EQ(initial_rank, sdsl::detail::rank_in_words(std::span<uint64_t const>{}, 0, initial_rank));
}
