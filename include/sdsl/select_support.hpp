// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
/*!\file select_support.hpp
 * \brief select_support.hpp contains classes that support a sdsl::bit_vector with constant time select information.
 * \author Simon Gog
 */
#ifndef INCLUDED_SDSL_SELECT_SUPPORT
#define INCLUDED_SDSL_SELECT_SUPPORT

/** \defgroup select_support_group Select Support (SCS)
 * This group contains data structures which support an sdsl::bit_vector with the select method.
 */

#include <iosfwd>
#include <stdint.h>
#include <string>

#include <sdsl/bits.hpp>
#include <sdsl/int_vector.hpp>
#include <sdsl/rank_support.hpp>
#include <sdsl/util.hpp>

//! Namespace for the succinct data structure library.
namespace sdsl
{
class structure_tree_node;

//! The base class of classes supporting select queries for a sdsl::bit_vector in constant time.
/*! Abstract base class for classes supporting select queries.
 */
class select_support
{
protected:
    int_vector<1> const * m_v; //!< Pointer to the select supported sdsl::bit_vector.

public:
    typedef int_vector<1>::size_type size_type;
    bit_vector const * vv;

    //! Constructor of select_support.
    /*!\param v The bit_vector to support rank queries.
     */
    select_support(int_vector<1> const * f_v = nullptr) : vv(f_v)
    {
        m_v = f_v;
    }
    //! Copy constructor
    /*! Copy the whole select_support including the  pointer
     *  to the supported bit_vector.
     */
    select_support(select_support const & f_v);
    //! Destructor of select_support.
    virtual ~select_support(){};

    //! Select returns the index of the i-th 1-bit in the supported bit_vector.
    /*!	\param i Argument to calculate the index of the i-th 1-bit in the supported bit_vector.
     * \return The index \f$\in [0..v.size()-1]\f$ of the i-th 1-bit in the supported bit_vector.
     * Call init or load to initialize the data structure before the first call of this method.
     * \sa init, load.
     */
    virtual size_type select(size_type i) const = 0;

    //! Alias for select
    virtual size_type operator()(size_type i) const = 0;
    //! Serialize the select_support to an out file stream.
    virtual size_type serialize(std::ostream & out, structure_tree_node * v, std::string name) const = 0;
    //! Load the select_support from an in file stream.
    /*!	Load an previously serialized select_support from a std::istream.
     * \param in The std::istream to load the select_support.
     * \param v The bit_vector to be supported.
     * \sa init, select.
     */
    virtual void load(std::istream & in, int_vector<1> const * v = nullptr) = 0;

    //! This method sets the supported bit_vector
    virtual void set_vector(int_vector<1> const * v = nullptr) = 0;
};

template <uint8_t bit_pattern, uint8_t pattern_len>
struct select_support_trait
{
    typedef select_support::size_type size_type;

    /* Count the number of arguments for the specific select support */
    static size_type arg_cnt(bit_vector const &)
    {
        return 0;
    }

    static size_type args_in_the_first_word(uint64_t, uint8_t, uint64_t)
    {
        return 0;
    }

    static size_type ith_arg_pos_in_the_first_word(uint64_t, size_type, uint8_t, uint64_t)
    {
        return 0;
    }

    static size_type args_in_the_word(uint64_t, uint64_t &)
    {
        return 0;
    }

    static size_type ith_arg_pos_in_the_word(uint64_t, size_type, uint64_t)
    {
        return 0;
    }

    static bool found_arg(size_type, bit_vector const &)
    {
        return 0;
    }

    static uint64_t init_carry(uint64_t const *, size_type)
    {
        return 0;
    }

    static uint64_t get_carry(uint64_t)
    {
        return 0;
    }
};

template <>
struct select_support_trait<0, 1>
{
    typedef select_support::size_type size_type;

    static size_type arg_cnt(bit_vector const & v)
    {
        return v.bit_size() - util::cnt_one_bits(v);
    }
    static size_type args_in_the_first_word(uint64_t w, uint8_t offset, uint64_t)
    {
        return bits::cnt((~w) & bits::lo_unset[offset]);
    }
    static size_type ith_arg_pos_in_the_first_word(uint64_t w, size_type i, uint8_t offset, uint64_t)
    {
        return bits::sel(~w & bits::lo_unset[offset], (uint32_t)i);
    }
    static size_type args_in_the_word(uint64_t w, uint64_t &)
    {
        return bits::cnt(~w);
    }
    static size_type ith_arg_pos_in_the_word(uint64_t w, size_type i, uint64_t)
    {
        return bits::sel(~w, (uint32_t)i);
    }
    static bool found_arg(size_type i, bit_vector const & v)
    {
        return !v[i];
    }
    static uint64_t init_carry(uint64_t const *, size_type)
    {
        return 0;
    }
    static uint64_t get_carry(uint64_t)
    {
        return 0;
    }
};

template <>
struct select_support_trait<1, 1>
{
    typedef select_support::size_type size_type;

    static size_type arg_cnt(bit_vector const & v)
    {
        return util::cnt_one_bits(v);
    }
    static size_type args_in_the_first_word(uint64_t w, uint8_t offset, uint64_t)
    {
        return bits::cnt(w & bits::lo_unset[offset]);
    }
    static size_type ith_arg_pos_in_the_first_word(uint64_t w, size_type i, uint8_t offset, uint64_t)
    {
        return bits::sel(w & bits::lo_unset[offset], (uint32_t)i);
    }
    static size_type args_in_the_word(uint64_t w, uint64_t &)
    {
        return bits::cnt(w);
    }
    static size_type ith_arg_pos_in_the_word(uint64_t w, size_type i, uint64_t)
    {
        return bits::sel(w, (uint32_t)i);
    }
    static bool found_arg(size_type i, bit_vector const & v)
    {
        return v[i] == 1;
    }
    static uint64_t init_carry(uint64_t const *, size_type)
    {
        return 0;
    }
    static uint64_t get_carry(uint64_t)
    {
        return 0;
    }
};

template <>
struct select_support_trait<10, 2>
{
    typedef select_support::size_type size_type;

    static size_type arg_cnt(bit_vector const & v)
    {
        return util::cnt_onezero_bits(v);
    }
    static size_type args_in_the_first_word(uint64_t w, uint8_t offset, uint64_t carry)
    {
        return bits::cnt(bits::map10(w, carry) & bits::lo_unset[offset]);
    }
    static size_type ith_arg_pos_in_the_first_word(uint64_t w, size_type i, uint8_t offset, uint64_t carry)
    {
        return bits::sel(bits::map10(w, carry) & bits::lo_unset[offset], (uint32_t)i);
    }
    static size_type args_in_the_word(uint64_t w, uint64_t & carry)
    {
        return bits::cnt10(w, carry);
    }
    static size_type ith_arg_pos_in_the_word(uint64_t w, size_type i, uint64_t carry)
    {
        return bits::sel(bits::map10(w, carry), (uint32_t)i);
    }
    static bool found_arg(size_type i, bit_vector const & v)
    {
        if (i > 0 and v[i - 1] and !v[i])
            return true;
        return false;
    }
    static uint64_t init_carry(uint64_t const * data, size_type word_pos)
    {
        return word_pos ? (*(data - 1) >> 63) : 0;
    }
    static uint64_t get_carry(uint64_t w)
    {
        return w >> 63;
    }
};

template <>
struct select_support_trait<01, 2>
{
    typedef select_support::size_type size_type;

    static size_type arg_cnt(bit_vector const & v)
    {
        return util::cnt_zeroone_bits(v);
    }
    static size_type args_in_the_first_word(uint64_t w, uint8_t offset, uint64_t carry)
    {
        return bits::cnt(bits::map01(w, carry) & bits::lo_unset[offset]);
    }
    static size_type ith_arg_pos_in_the_first_word(uint64_t w, size_type i, uint8_t offset, uint64_t carry)
    {
        return bits::sel(bits::map01(w, carry) & bits::lo_unset[offset], (uint32_t)i);
    }
    static size_type args_in_the_word(uint64_t w, uint64_t & carry)
    {
        return bits::cnt01(w, carry);
    }
    static size_type ith_arg_pos_in_the_word(uint64_t w, size_type i, uint64_t carry)
    {
        return bits::sel(bits::map01(w, carry), (uint32_t)i);
    }
    static bool found_arg(size_type i, bit_vector const & v)
    {
        if (i > 0 and !v[i - 1] and v[i])
            return true;
        return false;
    }
    static uint64_t init_carry(uint64_t const * data, size_type word_pos)
    {
        return word_pos ? (*(data - 1) >> 63) : 1;
    }
    static uint64_t get_carry(uint64_t w)
    {
        return w >> 63;
    }
};

template <>
struct select_support_trait<00, 2>
{
    typedef select_support::size_type size_type;

    static size_type arg_cnt(bit_vector const & v)
    {
        uint64_t const * data = v.data();
        if (v.empty())
            return 0;
        uint64_t carry = rank_support_trait<00, 2>::init_carry();
        size_type result = 0;
        for (auto end = v.data() + (v.size() >> 6); data < end; ++data)
        {
            result += rank_support_trait<00, 2>::args_in_the_word(*data, carry);
        }
        if (v.bit_size() & 0x3F)
        { // if bit_size is not a multiple of 64, add count of the last word
            result += rank_support_trait<00, 2>::args_in_the_word((*data) | bits::lo_unset[v.bit_size() & 0x3F], carry);
        }
        return result;
    }

    static size_type args_in_the_first_word(uint64_t w, uint8_t offset, uint64_t carry)
    {
        size_type res = 0;
        if (offset == 0)
            res = rank_support_trait<00, 2>::args_in_the_word(w, carry);
        else
        {
            res = bits::cnt((~(w | (w << 1))) & bits::lo_unset[offset]);
        }
        return res;
    }

    static size_type ith_arg_pos_in_the_first_word(uint64_t w, size_type i, uint8_t offset, uint64_t carry)
    {
        return bits::sel((~(((w << 1) | carry) | w)) & bits::lo_unset[offset], i);
    }
    static size_type args_in_the_word(uint64_t w, uint64_t & carry)
    {
        return rank_support_trait<00, 2>::args_in_the_word(w, carry);
    }
    static size_type ith_arg_pos_in_the_word(uint64_t w, size_type i, uint64_t carry)
    {
        return bits::sel(~(((w << 1) | carry) | w), i);
    }
    static bool found_arg(size_type i, bit_vector const & v)
    {
        return i > 0 and !v[i - 1] and !v[i];
    }
    static uint64_t init_carry(uint64_t const * data, size_type word_pos)
    {
        return word_pos ? (*(data - 1) >> 63) : 1;
    }
    static uint64_t get_carry(uint64_t w)
    {
        return w >> 63;
    }
};

template <>
struct select_support_trait<11, 2>
{
    typedef select_support::size_type size_type;

    static size_type arg_cnt(bit_vector const & v)
    {
        uint64_t const * data = v.data();
        if (v.empty())
            return 0;
        uint64_t carry = rank_support_trait<11, 2>::init_carry();
        size_type result = 0;
        for (auto end = v.data() + (v.size() >> 6); data < end; ++data)
        {
            result += rank_support_trait<11, 2>::args_in_the_word(*data, carry);
        }
        if (v.bit_size() & 0x3F)
        { // if bit_size is not a multiple of 64, add count of the last word
            result += rank_support_trait<11, 2>::args_in_the_word((*data) & bits::lo_set[v.bit_size() & 0x3F], carry);
        }
        return result;
    }

    static size_type args_in_the_first_word(uint64_t w, uint8_t offset, uint64_t carry)
    {
        size_type res = 0;
        if (offset == 0)
            res = rank_support_trait<11, 2>::args_in_the_word(w, carry);
        else
        {
            res = bits::cnt((w >> (offset - 1)) & (w >> offset));
        }
        return res;
    }

    static size_type ith_arg_pos_in_the_first_word(uint64_t w, size_type i, uint8_t offset, uint64_t carry)
    {
        return bits::sel((((w << 1) | carry) & w) & bits::lo_unset[offset], i);
    }
    static size_type args_in_the_word(uint64_t w, uint64_t & carry)
    {
        return rank_support_trait<11, 2>::args_in_the_word(w, carry);
    }
    static size_type ith_arg_pos_in_the_word(uint64_t w, size_type i, uint64_t carry)
    {
        return bits::sel(((w << 1) | carry) & w, i);
    }
    static bool found_arg(size_type i, bit_vector const & v)
    {
        if (i > 0 and v[i - 1] and v[i])
            return true;
        return false;
    }
    static uint64_t init_carry(uint64_t const * data, size_type word_pos)
    {
        return word_pos ? (*(data - 1) >> 63) : 0;
    }
    static uint64_t get_carry(uint64_t w)
    {
        return w >> 63;
    }
};

} // end namespace sdsl

#endif
