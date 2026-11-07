#ifndef SDSL_UNALIGNED_ACCESS_HPP
#define SDSL_UNALIGNED_ACCESS_HPP
#include <cstring>
#include <type_traits>

namespace sdsl::detail
{
// Read a value without requiring its serialized address to be aligned.
template <class Word>
inline Word load_unaligned(void const * address) noexcept
{
    static_assert(std::is_trivially_copyable_v<Word>);
    Word value;
    std::memcpy(&value, address, sizeof(value));
    return value;
}

template <class Word>
constexpr Word load_unaligned(Word const * address) noexcept
{
    if (std::is_constant_evaluated())
        return *address;
    return load_unaligned<Word>(static_cast<void const *>(address));
}
} // namespace sdsl::detail
#endif
