// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
/*!\file packed_vector_view.hpp
 * \brief packed_vector_view.hpp contains packed_vector_view, immutable packed storage that copies share.
 */
#ifndef SDSL_PACKED_VECTOR_VIEW_HPP
#define SDSL_PACKED_VECTOR_VIEW_HPP
#include <algorithm>
#include <cassert>
#include <cstring>
#include <istream>
#include <memory>
#include <ostream>
#include <ranges>
#include <span>
#include <stdexcept>
#include <utility>

#include <sdsl/int_vector.hpp>
#include <sdsl/iterators.hpp>
#include <sdsl/packed_word_iterator.hpp>
#include <sdsl/shared_mapper.hpp>
#include <sdsl/structure_tree.hpp>
namespace sdsl
{
// Immutable packed storage. Payloads contain whole 64-bit words but need not
// be aligned in memory. Copies retain the shared storage. Iterators and
// rank/select supports refer to the view object, not to its storage: the
// object must outlive them, and once it is moved from they see an empty view,
// although the storage lives on in the moved-to view. Iterators of int_vector
// follow the storage instead.
// No mutable access is exposed. load() and serialize() use the int_vector<Width>
// format; load() maps the payload from a shared_mapper_ifstream and copies it
// from any other stream. Only an explicit shared_mapper_ifstream maps:
// load_from_file() opens an isfstream and therefore copies. There is no cereal
// support, so types with view storage cannot be serialized with cereal.
template <uint8_t Width = 0>
class packed_vector_view : public std::ranges::view_interface<packed_vector_view<Width>>
{
    static_assert(Width <= 64);
    using vector_type = int_vector<Width>;
    // Deleter of the int_vector a view takes over. into_owned() looks for it.
    struct owned_deleter
    {
        void operator()(vector_type const * p) const noexcept
        {
            delete p;
        }
    };
    std::shared_ptr<void const> owner_;
    std::byte const * bytes_ = nullptr;
    uint64_t bits_ = 0;
    uint8_t width_ = Width ? Width : 64;

public:
    // View-backed rank supports, as int_vector names its owned ones.
    typedef rank_support_v<1, 1, packed_vector_view, sdsl::packed_vector_view> rank_1_type;
    typedef rank_support_v<0, 1, packed_vector_view, sdsl::packed_vector_view> rank_0_type;
    using size_type = uint64_t;
    using value_type = uint64_t;
    using difference_type = ptrdiff_t;
    // As int_vector: the width if it is fixed, otherwise 0.
    static constexpr uint8_t fixed_int_width = Width;
    packed_vector_view() = default;
    packed_vector_view(packed_vector_view const &) = default;
    packed_vector_view(packed_vector_view && v) noexcept
    {
        swap(v);
    }
    packed_vector_view & operator=(packed_vector_view v) noexcept
    {
        swap(v);
        return *this;
    }
    explicit packed_vector_view(vector_type value)
    {
        std::shared_ptr<vector_type const> p(new vector_type(std::move(value)), owned_deleter{});
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
    // Throws std::ios_base::failure if reading fails and, from a
    // shared_mapper_ifstream, std::invalid_argument if the payload exceeds the file.
    void load(std::istream & in)
    {
        auto * mapped = dynamic_cast<shared_mapper_ifstream *>(&in);
        if (!mapped || !mapped->mapper())
        {
            vector_type value;
            value.load(in);
            if (!in)
                throw std::ios_base::failure("packed_vector_view: cannot load");
            *this = packed_vector_view(std::move(value));
            return;
        }
        int_vector_size_type bits = 0;
        uint8_t width = Width;
        vector_type::read_header(bits, width, in);
        auto const pos = in.tellg();
        auto const & mapper = mapped->mapper();
        std::span file(reinterpret_cast<std::byte const *>(mapper->data()), mapper->size());
        // The stream can reach past the mapping if the file grew after it was mapped.
        if (pos < 0 or static_cast<uint64_t>(pos) > file.size())
            throw std::ios_base::failure("packed_vector_view: cannot read header");
        *this = from_bytes(mapper, file.subspan(pos), bits, width);
        in.seekg(bytes().size(), std::ios_base::cur);
    }
    uint64_t serialize(std::ostream & out, structure_tree_node * v = nullptr, std::string name = "") const
    {
        structure_tree_node * child = structure_tree::add_child(v, name, util::class_name(*this));
        uint64_t written = vector_type::write_header(bits_, width(), out);
        auto const payload = bytes();
        if (!payload.empty())
            out.write(reinterpret_cast<char const *>(payload.data()), payload.size());
        written += payload.size();
        structure_tree::add_size(child, written);
        return written;
    }
    void swap(packed_vector_view & v) noexcept
    {
        owner_.swap(v.owner_);
        std::swap(bytes_, v.bytes_);
        std::swap(bits_, v.bits_);
        std::swap(width_, v.width_);
    }
    uint64_t size() const
    {
        return bits_ / width();
    }
    uint64_t bit_size() const
    {
        return bits_;
    }
    uint64_t capacity() const
    {
        return ((bits_ + 63) >> 6) << 6;
    }
    uint8_t width() const
    {
        return Width ? Width : width_;
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
        // As bits::read_int, but never reads past the last payload word.
        auto const word = data() + (bit >> 6);
        uint64_t const offset = bit & 63;
        uint64_t value = *word >> offset;
        if (offset + length > 64)
            value |= word[1] << (64 - offset);
        return value & bits::lo_set[length];
    }
    uint64_t operator[](uint64_t i) const
    {
        assert(i < size());
        return get_int(i * width(), width());
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
    // Leaves the view empty. Returns the int_vector this view was constructed
    // from without copying it if no other view shares it, otherwise a copy.
    vector_type into_owned() &&
    {
        vector_type v;
        if (owner_.use_count() == 1 && std::get_deleter<owned_deleter>(owner_))
            v = std::move(*static_cast<vector_type *>(const_cast<void *>(owner_.get())));
        else
            v = to_owned();
        *this = packed_vector_view();
        return v;
    }
    using const_iterator = random_access_const_iterator<packed_vector_view>;
    using iterator = const_iterator;
    iterator begin() const
    {
        return {this, 0};
    }
    iterator end() const
    {
        return {this, size()};
    }
    bool operator==(packed_vector_view const & v) const
    {
        if (bits_ != v.bits_ || width_ != v.width_)
            return false;
        for (uint64_t bit = 0; bit < bits_; bit += 64)
        {
            uint8_t const length = std::min<uint64_t>(64, bits_ - bit);
            if (get_int(bit, length) != v.get_int(bit, length))
                return false;
        }
        return true;
    }
};
static_assert(std::ranges::random_access_range<packed_vector_view<1>>);
static_assert(!std::ranges::borrowed_range<packed_vector_view<1>>);
} // namespace sdsl
#endif
