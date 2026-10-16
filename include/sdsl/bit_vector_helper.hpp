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

} // namespace sdsl::detail
#endif
