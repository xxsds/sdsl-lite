#ifndef SDSL_PACKED_WORD_ITERATOR_HPP
#define SDSL_PACKED_WORD_ITERATOR_HPP
#include <compare>
#include <cstddef>
#include <cstdint>
#include <cstring>
namespace sdsl
{
// A read-only word iterator over packed storage that need not be aligned.
class packed_word_iterator
{
    std::byte const * p_ = nullptr;

public:
    explicit packed_word_iterator(std::byte const * p) : p_(p)
    {}
    uint64_t operator*() const
    {
        uint64_t word;
        std::memcpy(&word, p_, sizeof(word));
        return word;
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
    friend packed_word_iterator operator+(packed_word_iterator p, ptrdiff_t n)
    {
        return packed_word_iterator(p.p_ + n * 8);
    }
    friend packed_word_iterator operator-(packed_word_iterator p, ptrdiff_t n)
    {
        return packed_word_iterator(p.p_ - n * 8);
    }
    auto operator<=>(packed_word_iterator const &) const = default;
};
} // namespace sdsl
#endif
