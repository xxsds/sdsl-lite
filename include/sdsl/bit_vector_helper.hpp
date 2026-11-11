// Copyright (c) 2026, the SDSL Project Authors. All rights reserved.
// Please see the AUTHORS file for details. Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
#ifndef INCLUDED_SDSL_BIT_VECTOR_HELPER
#define INCLUDED_SDSL_BIT_VECTOR_HELPER

#include <cassert>
#include <cstdint>
#include <cstring>
#include <span>
#include <type_traits>

namespace sdsl::detail
{
// Index counts values of the requested type, independently of the storage type.
template <typename Value, typename T, size_t extent>
inline Value read_unaligned(std::span<T, extent> data, size_t index = 0)
{
    static_assert(std::is_trivially_copyable_v<Value>);
    assert(index < data.size_bytes() / sizeof(Value));
    Value value;
    auto bytes = std::as_bytes(data);
    std::memcpy(&value, &bytes[sizeof(Value) * index], sizeof(value));
    return value;
}

// Index counts values of the requested type, independently of the storage type.
template <typename Value, typename T, size_t extent>
inline void write_unaligned(std::span<T, extent> data, Value value, size_t index = 0)
{
    static_assert(std::is_trivially_copyable_v<Value>);
    assert(index < data.size_bytes() / sizeof(Value));
    auto bytes = std::as_writable_bytes(data);
    std::memcpy(&bytes[sizeof(Value) * index], &value, sizeof(value));
}

template <typename T, size_t extent>
inline bool access_in_words(std::span<T, extent> data, uint64_t position)
{
    return (read_unaligned<uint64_t>(data, position / 64) >> (position % 64)) & 1;
}

} // namespace sdsl::detail
#endif
