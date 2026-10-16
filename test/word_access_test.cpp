#include <array>
#include <cstring>
#include <span>
#include <sdsl/bit_vector_helper.hpp>
#include <gtest/gtest.h>

TEST(word_access_test, byte_and_word_storage)
{
    std::array<uint64_t, 4> words = {0, uint64_t{1} << 63, 0x5555555555555555ULL, ~uint64_t{0}};
    for (size_t offset = 0; offset < alignof(uint64_t); ++offset)
    {
        SCOPED_TRACE(offset);
        std::array<uint8_t, sizeof(words) + alignof(uint64_t) - 1> bytes;
        std::memcpy(bytes.data() + offset, words.data(), sizeof(words));
        auto data = std::span<uint8_t const>(bytes.data() + offset, sizeof(words));
        for (uint64_t position = 0; position < 256; ++position)
        {
            bool expected = (words[position / 64] >> (position % 64)) & 1;
            ASSERT_EQ(expected, sdsl::detail::access_in_words(std::span(words), position));
            ASSERT_EQ(expected, sdsl::detail::access_in_words(data, position));
        }
    }
}
