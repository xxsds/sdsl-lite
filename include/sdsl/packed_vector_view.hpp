#ifndef SDSL_PACKED_VECTOR_VIEW_HPP
#define SDSL_PACKED_VECTOR_VIEW_HPP
#include <cassert>
#include <memory>
#include <ranges>
#include <span>
#include <stdexcept>
#include <utility>

#include <sdsl/int_vector.hpp>
#include <sdsl/int_vector_mapper.hpp>
#include <sdsl/packed_word_iterator.hpp>
namespace sdsl
{
// Immutable packed storage, independent of any serialized header format.
// Payloads contain whole 64-bit words but need not be aligned in memory.
// Copies retain the shared storage. Iterators and rank/select supports borrow
// the view object, which must outlive them. No mutable access is exposed.
template <uint8_t Width = 0>
class packed_vector_view : public std::ranges::view_interface<packed_vector_view<Width>>
{
    static_assert(Width <= 64);
    using vector_type = int_vector<Width>;
    std::shared_ptr<void const> owner_;
    vector_type * owned_ = nullptr;
    std::byte const * bytes_ = nullptr;
    uint64_t bits_ = 0;
    uint8_t width_ = Width ? Width : 64;

public:
    using size_type = uint64_t;
    using value_type = uint64_t;
    using difference_type = ptrdiff_t;
    using byte_mapper = int_vector_mapper<8, std::ios_base::in>;
    packed_vector_view() = default;
    packed_vector_view(packed_vector_view const &) = default;
    packed_vector_view & operator=(packed_vector_view const &) = default;
    packed_vector_view(packed_vector_view && v) noexcept
    {
        swap(v);
    }
    packed_vector_view & operator=(packed_vector_view && v) noexcept
    {
        if (this != &v)
        {
            packed_vector_view tmp(std::move(v));
            swap(tmp);
        }
        return *this;
    }
    explicit packed_vector_view(vector_type value)
    {
        auto p = std::make_shared<vector_type>(std::move(value));
        owned_ = p.get();
        bytes_ = reinterpret_cast<std::byte const *>(p->data());
        bits_ = p->bit_size();
        width_ = p->width();
        owner_ = std::move(p);
    }
    // The caller must ensure owner keeps payload alive and immutable.
    static packed_vector_view from_bytes(std::shared_ptr<void const> owner,
                                         std::span<std::byte const> payload,
                                         uint64_t bit_size,
                                         uint8_t width = Width)
    {
        if (!width || width > 64 || (Width && width != Width) || bit_size > UINT64_MAX - 63)
            throw std::invalid_argument("Invalid packed view dimensions");
        auto needed = ((bit_size + 63) >> 6) * 8;
        if (needed > payload.size() || (needed && !owner))
            throw std::invalid_argument("Packed view exceeds owned storage");
        packed_vector_view v;
        v.owner_ = std::move(owner);
        v.bytes_ = payload.data();
        v.bits_ = bit_size;
        v.width_ = width;
        return v;
    }
    static packed_vector_view
    from_mapping(std::shared_ptr<byte_mapper const> owner, uint64_t offset, uint64_t bit_size, uint8_t width = Width)
    {
        if (!owner || offset > owner->size())
            throw std::out_of_range("Packed view offset");
        std::span bytes(reinterpret_cast<std::byte const *>(owner->data()), owner->size());
        return from_bytes(owner, bytes.subspan(offset), bit_size, width);
    }
    void swap(packed_vector_view & v) noexcept
    {
        owner_.swap(v.owner_);
        std::swap(owned_, v.owned_);
        std::swap(bytes_, v.bytes_);
        std::swap(bits_, v.bits_);
        std::swap(width_, v.width_);
    }
    uint64_t size() const
    {
        return bits_ / width_;
    }
    uint64_t bit_size() const
    {
        return bits_;
    }
    uint64_t capacity() const
    {
        return ((bits_ + 63) >> 6) << 6;
    }
    bool empty() const
    {
        return !size();
    }
    uint8_t width() const
    {
        return width_;
    }
    std::span<std::byte const> bytes() const
    {
        return {bytes_, ((bits_ + 63) >> 6) * 8};
    }
    packed_word_iterator data() const
    {
        return packed_word_iterator(bytes_);
    }
    uint64_t get_int(uint64_t bit, uint8_t length = 64) const
    {
        assert(length <= 64 && bit <= bits_ && length <= bits_ - bit);
        if (!length)
            return 0;
        auto p = data() + (bit >> 6);
        auto shift = bit & 63;
        auto result = *p >> shift;
        if (shift + length > 64)
            result |= p[1] << (64 - shift);
        return result & bits::lo_set[length];
    }
    uint64_t operator[](uint64_t i) const
    {
        assert(i < size());
        return get_int(i * width_, width_);
    }
    vector_type to_owned() const
    {
        vector_type v;
        v.width(width_);
        v.bit_resize(bits_);
        if (bits_)
            std::memcpy(v.data(), bytes_, bytes().size());
        return v;
    }
    vector_type into_owned() &&
    {
        // Transfer a unique allocation; mapped or shared storage needs a copy.
        if (owned_ && owner_.use_count() == 1)
        {
            auto v = std::move(*owned_);
            *this = packed_vector_view();
            return v;
        }
        return to_owned();
    }
    class iterator
    {
        packed_vector_view const * v_ = nullptr;
        ptrdiff_t i_ = 0;

    public:
        using iterator_category = std::random_access_iterator_tag;
        using iterator_concept = std::random_access_iterator_tag;
        using value_type = uint64_t;
        using difference_type = ptrdiff_t;
        iterator() = default;
        iterator(packed_vector_view const * v, ptrdiff_t i) : v_(v), i_(i)
        {}
        uint64_t operator*() const
        {
            return (*v_)[i_];
        }
        uint64_t operator[](ptrdiff_t n) const
        {
            return (*v_)[i_ + n];
        }
        iterator & operator++()
        {
            ++i_;
            return *this;
        }
        iterator operator++(int)
        {
            auto x = *this;
            ++*this;
            return x;
        }
        iterator & operator--()
        {
            --i_;
            return *this;
        }
        iterator operator--(int)
        {
            auto x = *this;
            --*this;
            return x;
        }
        iterator & operator+=(ptrdiff_t n)
        {
            i_ += n;
            return *this;
        }
        iterator & operator-=(ptrdiff_t n)
        {
            i_ -= n;
            return *this;
        }
        friend iterator operator+(iterator p, ptrdiff_t n)
        {
            return p += n;
        }
        friend iterator operator+(ptrdiff_t n, iterator p)
        {
            return p += n;
        }
        friend iterator operator-(iterator p, ptrdiff_t n)
        {
            return p -= n;
        }
        friend ptrdiff_t operator-(iterator a, iterator b)
        {
            assert(a.v_ == b.v_);
            return a.i_ - b.i_;
        }
        friend bool operator==(iterator a, iterator b)
        {
            return a.v_ == b.v_ && a.i_ == b.i_;
        }
        friend auto operator<=>(iterator a, iterator b)
        {
            assert(a.v_ == b.v_);
            return a.i_ <=> b.i_;
        }
    };
    iterator begin() const
    {
        return {this, 0};
    }
    iterator end() const
    {
        return {this, static_cast<ptrdiff_t>(size())};
    }
    bool operator==(packed_vector_view const & v) const
    {
        if (bits_ != v.bits_ || width_ != v.width_)
            return false;
        for (uint64_t p = 0; p < bits_; p += 64)
            if (get_int(p, std::min<uint64_t>(64, bits_ - p)) != v.get_int(p, std::min<uint64_t>(64, bits_ - p)))
                return false;
        return true;
    }
};
static_assert(std::ranges::random_access_range<packed_vector_view<1>>);
static_assert(!std::ranges::borrowed_range<packed_vector_view<1>>);
} // namespace sdsl
#endif
