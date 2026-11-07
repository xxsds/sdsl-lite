#ifndef SDSL_PACKED_WORD_ITERATOR_HPP
#define SDSL_PACKED_WORD_ITERATOR_HPP
#include <compare>
#include <cstddef>
#include <cstdint>
#include <sdsl/unaligned_access.hpp>
namespace sdsl
{
// A word reader for potentially unaligned, word-padded packed storage.
// It deliberately exposes no writable or uint64_t pointer conversion.
class packed_word_iterator
{
    std::byte const * p_ = nullptr;

public:
    packed_word_iterator() = default;
    explicit packed_word_iterator(std::byte const * p) : p_(p)
    {}
    uint64_t operator*() const
    {
        return detail::load_unaligned<uint64_t>(static_cast<void const *>(p_));
    }
    uint64_t operator[](ptrdiff_t n) const
    {
        return *(*this + n);
    }
    packed_word_iterator & operator++()
    {
        p_ += 8;
        return *this;
    }
    packed_word_iterator operator++(int)
    {
        auto v = *this;
        ++*this;
        return v;
    }
    packed_word_iterator & operator--()
    {
        p_ -= 8;
        return *this;
    }
    packed_word_iterator & operator+=(ptrdiff_t n)
    {
        if (n)
            p_ += n * 8;
        return *this;
    }
    packed_word_iterator & operator-=(ptrdiff_t n)
    {
        return *this += -n;
    }
    friend packed_word_iterator operator+(packed_word_iterator p, ptrdiff_t n)
    {
        return p += n;
    }
    friend packed_word_iterator operator-(packed_word_iterator p, ptrdiff_t n)
    {
        return p -= n;
    }
    friend ptrdiff_t operator-(packed_word_iterator a, packed_word_iterator b)
    {
        return (a.p_ - b.p_) / 8;
    }
    auto operator<=>(packed_word_iterator const &) const = default;
};
namespace detail
{
inline uint64_t load_unaligned(packed_word_iterator p)
{
    return *p;
}
} // namespace detail
} // namespace sdsl
#endif
