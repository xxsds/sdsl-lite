#include <array>
#include <cstring>
#include <span>
#include <sdsl/bit_vector_helper.hpp>
#include <gtest/gtest.h>

TEST(word_select_test, byte_and_word_storage)
{
    std::array<uint64_t, 4> words = {0, uint64_t{1} << 63, 0x5555555555555555ULL, ~uint64_t{0}};
    for (size_t offset = 0; offset < alignof(uint64_t); ++offset)
    {
        SCOPED_TRACE(offset);
        std::array<uint8_t, sizeof(words) + alignof(uint64_t) - 1> bytes;
        std::memcpy(bytes.data() + offset, words.data(), sizeof(words));
        auto data = std::span<uint8_t const>(bytes.data() + offset, sizeof(words));
        uint64_t ones = 0;
        uint64_t zeros = 0;
        for (uint64_t position = 0; position < 256; ++position)
        {
            if ((words[position / 64] >> (position % 64)) & 1)
            {
                ++ones;
                ASSERT_EQ(position, sdsl::detail::select_in_words<1>(std::span(words), ones));
                ASSERT_EQ(position, sdsl::detail::select_in_words<1>(data, ones));
            }
            else
            {
                ++zeros;
                ASSERT_EQ(position, sdsl::detail::select_in_words<0>(std::span(words), zeros));
                ASSERT_EQ(position, sdsl::detail::select_in_words<0>(data, zeros));
            }
        }
    }
}
