// Copyright (c) 2026, the SDSL Project Authors. All rights reserved.
// Please see the AUTHORS file for details. Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
#ifndef INCLUDED_SDSL_BIT_VECTOR_HELPER
#define INCLUDED_SDSL_BIT_VECTOR_HELPER

#include <cassert>
#include <cstring>
#include <span>
#include <type_traits>

#include <sdsl/bits.hpp>

namespace sdsl::detail
{
// Index counts values of the requested type, independently of the storage type.
template <typename Value, typename T, size_t extent>
inline Value read_unaligned(std::span<T, extent> data, size_t index = 0)
{
    static_assert(std::is_trivially_copyable_v<Value>);
    assert(index < data.size_bytes() / sizeof(Value));
    Value value;
    std::memcpy(&value, std::as_bytes(data).data() + sizeof(Value) * index, sizeof(value));
    return value;
}

template <typename T, size_t extent>
inline bool access_in_words(std::span<T, extent> data, uint64_t position)
{
    return (read_unaligned<uint64_t>(data, position / 64) >> (position % 64)) & 1;
}

// Count a nonempty prefix of at most t_words words.
template <size_t t_words, typename T, size_t extent>
#if __has_cpp_attribute(gnu::flatten)
[[gnu::flatten]]
#endif
inline uint64_t rank_word_prefix(std::span<T, extent> data, uint32_t prefix_length, uint64_t ones)
{
    static_assert(t_words > 0);
    assert(prefix_length > 0 && prefix_length <= 64 * t_words);
    auto value = read_unaligned<uint64_t>(data);
    if constexpr (t_words > 1)
        if (prefix_length > 64)
            return rank_word_prefix<t_words - 1>(std::as_bytes(data).subspan(8), prefix_length - 64,
                                                ones + bits::cnt(value));
    return ones + bits::cnt(value << (64 - prefix_length));
}

template <typename T, size_t extent>
inline uint64_t rank_in_words(std::span<T, extent> data, uint64_t prefix_length, uint64_t ones = 0)
{
    assert(prefix_length <= data.size_bytes() * 8);
    if (!prefix_length)
        return ones;
    // Bound expansion when bits::cnt uses its software implementation.
#ifdef __SSE4_2__
    constexpr size_t long_block_words = 7;
#else
    constexpr size_t long_block_words = 1;
#endif
    constexpr size_t block_words = extent != std::dynamic_extent && extent * sizeof(T) <= 32
                                 ? 4 : long_block_words;
    std::span<std::byte const> bytes = std::as_bytes(data);
    for (; prefix_length > 64 * block_words; prefix_length -= 64 * block_words, bytes = bytes.subspan(8 * block_words))
        ones = rank_word_prefix<block_words>(bytes, 64 * block_words, ones);
    return rank_word_prefix<block_words>(bytes, prefix_length, ones);
}

// The one-based occurrence must exist in the supplied words.
template <bool t_bit, typename T, size_t extent>
inline uint64_t select_in_words(std::span<T, extent> data, uint64_t occurrence)
{
    assert(occurrence > 0);
    for (size_t word = 0;; ++word)
    {
        uint64_t value = read_unaligned<uint64_t>(data, word);
        if constexpr (!t_bit)
            value = ~value;
        auto count = bits::cnt(value);
        if (occurrence <= count)
            return 64 * word + bits::sel(value, occurrence);
        occurrence -= count;
    }
}
} // namespace sdsl::detail
#endif
