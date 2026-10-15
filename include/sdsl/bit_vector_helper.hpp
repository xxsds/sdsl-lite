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
