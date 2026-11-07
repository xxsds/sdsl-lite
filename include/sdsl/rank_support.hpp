// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
/*!\file rank_support.hpp
 * \brief rank_support.hpp contains classes that support a sdsl::bit_vector with constant time rank information.
 * \author Simon Gog
 */
#ifndef INCLUDED_SDSL_RANK_SUPPORT
#define INCLUDED_SDSL_RANK_SUPPORT

/** \defgroup rank_support_group Rank Support (RS)
 * This group contains data structures which support an sdsl::bit_vector with the rank method.
 */

#include <iosfwd>
#include <stdint.h>
#include <string>

#include <sdsl/bits.hpp>
#include <sdsl/int_vector.hpp>

//! Namespace for the succinct data structure library.
namespace sdsl
{
class structure_tree_node;

//! The base class of classes supporting rank_queries for a sdsl::bit_vector in constant time.
/*!
 */
template <class Vector>
class rank_support_base
{
protected:
    Vector const * m_v; //!< Pointer to the rank supported bit_vector

public:
    typedef typename Vector::size_type size_type;

    //! Constructor
    /*!\param v The supported bit_vector.
     */
    rank_support_base(Vector const * v = nullptr);
    //! Copy constructor
    rank_support_base(rank_support_base const &) = default;
    rank_support_base(rank_support_base &&) = default;
    rank_support_base & operator=(rank_support_base const &) = default;
    rank_support_base & operator=(rank_support_base &&) = default;
    //! Destructor
    virtual ~rank_support_base()
    {}

    //! Answers rank queries for the supported bit_vector.
    /*!	\param i Argument for the length of the prefix v[0..i-1].
     * \returns Number of 1-bits in the prefix [0..i-1] of the supported bit_vector.
     * \note Method init has to be called before the first call of rank.
     * \sa init
     */
    virtual size_type rank(size_type i) const = 0;
    //! Alias for rank(i)
    virtual size_type operator()(size_type idx) const = 0;
    //! Serializes rank_support_base.
    /*!\param out Out-Stream to serialize the data to.
     */
    virtual size_type serialize(std::ostream & out, structure_tree_node * v, std::string name) const = 0;
    //! Loads the rank_support_base.
    /*!\param in In-Stream to load the rank_support_base data from.
     * \param v The supported bit_vector.
     */
    virtual void load(std::istream & in, Vector const * v = nullptr) = 0;
    //! Sets the supported bit_vector to the given pointer.
    /*!\param v The new bit_vector to support.
     *  \note Method init has to be called before the next call of rank.
     *  \sa init, rank
     */
    virtual void set_vector(Vector const * v = nullptr) = 0;
};

template <class Vector>
inline rank_support_base<Vector>::rank_support_base(Vector const * v)
{
    m_v = v;
}

//----------------------------------------------------------------------

using rank_support = rank_support_base<bit_vector>;

template <uint8_t bit_pattern, uint8_t pattern_len>
struct rank_support_trait
{
    typedef rank_support::size_type size_type;

    static size_type args_in_the_word(uint64_t, uint64_t &)
    {
        return 0;
    }

    template <class WordIterator>
    static uint32_t word_rank(WordIterator, size_type)
    {
        return 0;
    }

    template <class WordIterator>
    static uint32_t full_word_rank(WordIterator, size_type)
    {
        return 0;
    }

    static uint64_t init_carry()
    {
        return 0;
    }
};

template <>
struct rank_support_trait<0, 1>
{
    typedef rank_support::size_type size_type;

    static size_type args_in_the_word(uint64_t w, uint64_t &)
    {
        return bits::cnt(~w);
    }

    template <class WordIterator>
    static uint32_t word_rank(WordIterator data, size_type idx)
    {
        if (!(idx & 63))
            return 0;
        return bits::cnt((~*(data + (idx >> 6))) & bits::lo_set[idx & 0x3F]);
    }

    template <class WordIterator>
    static uint32_t full_word_rank(WordIterator data, size_type idx)
    {
        return bits::cnt((~*(data + (idx >> 6))));
    }

    static uint64_t init_carry()
    {
        return 0;
    }
};

template <>
struct rank_support_trait<1, 1>
{
    typedef rank_support::size_type size_type;

    static size_type args_in_the_word(uint64_t w, uint64_t &)
    {
        return bits::cnt(w);
    }

    template <class WordIterator>
    static uint32_t word_rank(WordIterator data, size_type idx)
    {
        if (!(idx & 63))
            return 0;
        return bits::cnt(*(data + (idx >> 6)) & bits::lo_set[idx & 0x3F]);
    }

    template <class WordIterator>
    static uint32_t full_word_rank(WordIterator data, size_type idx)
    {
        return bits::cnt(*(data + (idx >> 6)));
    }

    static uint64_t init_carry()
    {
        return 0;
    }
};

template <>
struct rank_support_trait<10, 2>
{
    typedef rank_support::size_type size_type;

    static size_type args_in_the_word(uint64_t w, uint64_t & carry)
    {
        return bits::cnt10(w, carry);
    }

    template <class WordIterator>
    static uint32_t word_rank(WordIterator data, size_type idx)
    {
        if (!(idx & 63))
            return 0;
        data = data + (idx >> 6);
        uint64_t carry = (idx > 63) ? *(data - 1) >> 63 : 0;
        return bits::cnt(bits::map10(*data, carry) & bits::lo_set[idx & 0x3F]);
    }

    template <class WordIterator>
    static uint32_t full_word_rank(WordIterator data, size_type idx)
    {
        data = data + (idx >> 6);
        uint64_t carry = (idx > 63) ? *(data - 1) >> 63 : 0;
        return bits::cnt(bits::map10(*data, carry));
    }

    static uint64_t init_carry()
    {
        return 0;
    }
};

template <>
struct rank_support_trait<01, 2>
{
    typedef rank_support::size_type size_type;

    static size_type args_in_the_word(uint64_t w, uint64_t & carry)
    {
        return bits::cnt01(w, carry);
    }

    template <class WordIterator>
    static uint32_t word_rank(WordIterator data, size_type idx)
    {
        if (!(idx & 63))
            return 0;
        data = data + (idx >> 6);
        uint64_t carry = (idx > 63) ? *(data - 1) >> 63 : 1;
        return bits::cnt(bits::map01(*data, carry) & bits::lo_set[idx & 0x3F]);
    }

    template <class WordIterator>
    static uint32_t full_word_rank(WordIterator data, size_type idx)
    {
        data = data + (idx >> 6);
        uint64_t carry = (idx > 63) ? *(data - 1) >> 63 : 1;
        return bits::cnt(bits::map01(*data, carry));
    }

    static uint64_t init_carry()
    {
        return 1;
    }
};

template <>
struct rank_support_trait<00, 2>
{
    typedef rank_support::size_type size_type;

    static size_type args_in_the_word(uint64_t w, uint64_t & carry)
    {
        size_type res = bits::cnt(~(w | (w << 1 | carry)));
        carry = (w >> 63);
        return res;
    }

    template <class WordIterator>
    static uint32_t word_rank(WordIterator data, size_type idx)
    {
        if (!(idx & 63))
            return 0;
        data = data + (idx >> 6);
        uint64_t carry = (idx > 63) ? *(data - 1) >> 63 : 1;
        return bits::cnt((~(*data | ((*data) << 1 | carry))) & bits::lo_set[idx & 0x3F]);
    }

    template <class WordIterator>
    static uint32_t full_word_rank(WordIterator data, size_type idx)
    {
        data = data + (idx >> 6);
        uint64_t carry = (idx > 63) ? *(data - 1) >> 63 : 1;
        return bits::cnt(~(*data | ((*data) << 1 | carry)));
    }

    static uint64_t init_carry()
    {
        return 1;
    }
};

template <>
struct rank_support_trait<11, 2>
{
    typedef rank_support::size_type size_type;

    static size_type args_in_the_word(uint64_t w, uint64_t & carry)
    {
        size_type res = bits::cnt(w & (w << 1 | carry));
        carry = (w >> 63);
        return res;
    }

    template <class WordIterator>
    static uint32_t word_rank(WordIterator data, size_type idx)
    {
        if (!(idx & 63))
            return 0;
        data = data + (idx >> 6);
        uint64_t carry = (idx > 63) ? *(data - 1) >> 63 : 0;
        return bits::cnt((*data & ((*data) << 1 | carry)) & bits::lo_set[idx & 0x3F]);
    }

    template <class WordIterator>
    static uint32_t full_word_rank(WordIterator data, size_type idx)
    {
        data = data + (idx >> 6);
        uint64_t carry = (idx > 63) ? *(data - 1) >> 63 : 0;
        return bits::cnt(*data & ((*data) << 1 | carry));
    }

    static uint64_t init_carry()
    {
        return 0;
    }
};

} // namespace sdsl

// clang-format off
// Cyclic includes start
#include <sdsl/rank_support_scan.hpp>
#include <sdsl/rank_support_v.hpp>
#include <sdsl/rank_support_v5.hpp>
// Cyclic includes end
// clang-format on

#endif
