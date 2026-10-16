// Copyright (c) 2026, the SDSL Project Authors. All rights reserved.
// Please see the AUTHORS file for details. Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
#ifndef INCLUDED_SDSL_BIT_VECTOR_HELPER
#define INCLUDED_SDSL_BIT_VECTOR_HELPER

#include <algorithm>
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

// Count ones in a prefix of the supplied words.
template <typename T, size_t extent>
inline uint64_t rank_in_words(std::span<T, extent> data, uint64_t prefix_length, uint64_t ones = 0)
{
    assert(prefix_length <= data.size_bytes() * 8);
    // Fixed groups avoid branches with hardware popcount; software counting skips unused words.
#ifdef __SSE4_2__
    constexpr bool fixed_group = true;
#else
    constexpr bool fixed_group = false;
#endif
    constexpr size_t block_words = extent != std::dynamic_extent && extent * sizeof(T) <= 32
                                 ? 4 : fixed_group ? 8 : 1;
    std::span<std::byte const> bytes = std::as_bytes(data);
    auto count = [&](uint32_t length)
    {
        uint32_t last = std::min<uint32_t>((length - 1) / 64, block_words - 1);
        auto tail = read_unaligned<uint64_t>(bytes, last);
        uint64_t sum = bits::cnt(tail << (64 * (last + 1) - length));
        // Clamp unused reads to the last word; their counts contribute zero.
#if defined(__GNUC__) || defined(__clang__)
#    pragma GCC unroll 8
#endif
        for (uint32_t word = 0; word < (fixed_group ? block_words - 1 : last); ++word)
            sum += bits::cnt(read_unaligned<uint64_t>(bytes, std::min(word, last))) * (word < last);
        return sum;
    };
    for (; prefix_length > 64 * block_words; prefix_length -= 64 * block_words, bytes = bytes.subspan(8 * block_words))
        ones += count(64 * block_words);
    return prefix_length ? ones + count(prefix_length) : ones;
}

} // namespace sdsl::detail
#endif
