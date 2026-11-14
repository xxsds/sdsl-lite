// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
/*!\file rrr_helper.hpp
 * \brief rrr_helper.hpp contains the sdsl::binomial struct,
 * a struct which contains informations about the binomial coefficients
 * \author Simon Gog, Matthias Petri, Stefan Arnold
 */
#ifndef SDSL_RRR_HELPER
#define SDSL_RRR_HELPER

#include <algorithm>
#include <stdint.h>
#include <utility>

#include <sdsl/bits.hpp>
#include <sdsl/uint128_t.hpp>
#include <sdsl/uint256_t.hpp>

namespace sdsl
{

//! Trait struct for the binomial coefficient struct to handle different type of integers.
/*! This generic implementation works for 64-bit integers.
 */
template <uint16_t log_n>
struct binomial_coefficients_trait
{
    typedef uint64_t number_type;
    static inline uint16_t hi(number_type x)
    {
        return bits::hi(x);
    }

    //! Read a \f$len\f$-bit integer of type number_type from a bitvector.
    /*!
     * \param bv   A bit_vector of int_vector from which we extract the integer.
     * \param pos  Position of the least significant bit of the integer which should be read.
     * \param len  bit-width of the integer which should be read.
     * \return The len-bit integer.
     */
    template <class bit_vector_type>
    static inline number_type get_int(bit_vector_type const & bv, typename bit_vector_type::size_type pos, uint16_t len)
    {
        return bv.get_int(pos, len);
    }

    //! Write a \f$len\f$-bit integer x of type number_type to a bitvector.
    /*!
     * \param bv     A bit_vecor or int_vector in which we write the integer.
     * \param pos    Position of the least significant bit of the integer which should be written.
     * \param x    The integer x which should be written.
     * \param len  Bit-width of x.
     */
    template <class bit_vector_type>
    static void set_int(bit_vector_type & bv, typename bit_vector_type::size_type pos, number_type x, uint16_t len)
    {
        bv.set_int(pos, x, len);
    }

    //! Count the number of set bits in x.
    /*!
     *  \param x The integer x.
     */
    static inline uint16_t popcount(number_type x)
    {
        return bits::cnt(x);
    }
};

//! Specialization of binomial_coefficients_trait for 128-bit integers.
template <>
struct binomial_coefficients_trait<7>
{
    typedef uint128_t number_type;
    static inline uint16_t hi(number_type x)
    {
        if ((x >> 64))
        {
            return bits::hi(x >> 64) + 64;
        }
        else
        {
            return bits::hi(x);
        }
    }

    template <class bit_vector_type>
    static inline number_type get_int(bit_vector_type const & bv, typename bit_vector_type::size_type pos, uint16_t len)
    {
        if (len <= 64)
        {
            return bv.get_int(pos, len);
        }
        else
        {
            return ((((number_type)bv.get_int(pos + 64, len - 64)) << 64) + bv.get_int(pos, 64));
        }
    }

    template <class bit_vector_type>
    static void set_int(bit_vector_type & bv, typename bit_vector_type::size_type pos, number_type x, uint16_t len)
    {
        if (len <= 64)
        {
            bv.set_int(pos, x, len);
        }
        else
        {
            bv.set_int(pos, (uint64_t)x, 64);
            bv.set_int(pos + 64, x >> 64, len - 64);
        }
    }

    static inline uint16_t popcount(number_type x)
    {
        return bits::cnt(x >> 64) + bits::cnt(x);
    }
};

//! Specialization of binomial_coefficients_trait for 256-bit integers.
template <>
struct binomial_coefficients_trait<8>
{
    typedef uint256_t number_type;
    static inline uint16_t hi(number_type x)
    {
        return x.hi();
    }

    template <class bit_vector_type>
    static inline number_type get_int(bit_vector_type const & bv, typename bit_vector_type::size_type pos, uint16_t len)
    {
        if (len <= 64)
        {
            return number_type(bv.get_int(pos, len));
        }
        else if (len <= 128)
        {
            return number_type(bv.get_int(pos, 64), bv.get_int(pos + 64, len - 64));
        }
        else if (len <= 192)
        {
            return number_type(bv.get_int(pos, 64),
                               bv.get_int(pos + 64, 64),
                               (uint128_t)bv.get_int(pos + 128, len - 128));
        }
        else
        { // > 192
            return number_type(bv.get_int(pos, 64),
                               bv.get_int(pos + 64, 64),
                               (((uint128_t)bv.get_int(pos + 192, len - 192)) << 64) | bv.get_int(pos + 128, 64));
        }
    }

    template <class bit_vector_type>
    static void set_int(bit_vector_type & bv, typename bit_vector_type::size_type pos, number_type x, uint16_t len)
    {
        if (len <= 64)
        {
            bv.set_int(pos, x, len);
        }
        else if (len <= 128)
        {
            bv.set_int(pos, x, 64);
            bv.set_int(pos + 64, x >> 64, len - 64);
        }
        else if (len <= 192)
        {
            bv.set_int(pos, x, 64);
            bv.set_int(pos + 64, x >> 64, 64);
            bv.set_int(pos + 128, x >> 128, len - 128);
        }
        else
        { // > 192
            bv.set_int(pos, x, 64);
            bv.set_int(pos + 64, x >> 64, 64);
            bv.set_int(pos + 128, x >> 128, 64);
            bv.set_int(pos + 192, x >> 192, len - 192);
        }
    }

    static inline uint16_t popcount(number_type x)
    {
        return x.popcount();
    }
};

template <uint16_t n, class number_type>
struct binomial_table
{
    static struct impl
    {
        // table[k][m] contains {m choose k}. Decoding walks along m for a fixed k, so m is the inner index.
        number_type table[n + 1][n + 1];
        // jump[k][t] is the largest m in [1..n] with {m-1 choose k} < 2^(t+1).
        // If a block suffix of length m holds k ones and its number is below 2^(t+1),
        // its first bit can only be set if m <= jump[k][t], so all longer suffixes start with zeros.
        uint16_t jump[n + 1][n];
        number_type L1Mask[n + 1]; // L1Mask[i] contains a word with the i least significant bits set to 1.
        // i.e. L1Mask[0] = 0, L1Mask[1] = 1,...
        number_type O1Mask[n]; // O1Mask[i] contains a word with the i least significant bits set to 0.

        impl()
        {
            for (uint16_t m = 0; m <= n; ++m)
            {
                table[0][m] = 1;
            }
            for (uint16_t k = 1; k <= n; ++k)
            {
                table[k][0] = 0;
                for (uint16_t m = 1; m <= n; ++m)
                {
                    table[k][m] = table[k - 1][m - 1] + table[k][m - 1];
                }
            }
            for (uint16_t k = 0; k <= n; ++k)
            {
                uint16_t m = 1; // jump[k][t] grows with t
                number_type bound = 0;
                for (uint16_t t = 0; t < n; ++t)
                {
                    bound = (bound << 1) | (number_type)1; // bound = 2^(t+1) - 1
                    while (m < n and table[k][m] <= bound)
                        ++m;
                    jump[k][t] = m;
                }
            }
            L1Mask[0] = 0;
            number_type mask = 1;
            O1Mask[0] = 1;
            for (int i = 1; i <= n; ++i)
            {
                L1Mask[i] = mask;
                if (i < n)
                    O1Mask[i] = O1Mask[i - 1] << 1;
                mask = (mask << 1);
                mask |= (number_type)1;
            }
        }
    } data;
};

template <uint16_t n, class number_type>
typename binomial_table<n, number_type>::impl binomial_table<n, number_type>::data;

//! A struct for the binomial coefficients \f$ n \choose k \f$.
/*!
 * data.table[k][m] contains the number \f${m \choose k}\f$ for \f$ k\leq m\leq n\f$.
 * Size of data.table :
 *   Let \f$ maxlog = \lceil \log n \rceil \f$ and \f$ maxsize = 2^{maxlog} \f$
 *   then the tables requires \f$ maxsize^2\times \lceil n/8 rceil \f$ bytes space.
 *    Examples:
 *          n <=  64: 64*64*8 bytes    =   4 kB *  8 = 32 kB
 *     64 < n <= 128: 128*128*16 bytes =  16 kB * 16 = 256 kB
 *    128 < n <= 256: 256*256*32 bytes =  64 kB * 32 = 2048 kB = 2 MB
 *  The table is shared now for all n's in on of these ranges.
 *
 * data.space[k] returns the bits needed to encode a value between [0..data.table[n][k]], given n and k.
 * Size of data.space is  \f$ (n+1) \times \lceil n/8 \rceil \f$ bytes. E.g. 64*8=512 bytes for n=63,
 * 2kB for n=127, and 8kB for n=255.
 *
 * data.jump[k][t] is the largest \f$ m \f$ with \f${m-1 \choose k} < 2^{t+1}\f$, see binomial_table.
 * \pre The template parameter n should be in the range [7..256].
 */
template <uint16_t n>
struct binomial_coefficients
{
    enum
    {
        MAX_LOG = (n > 128 ? 8 : (n > 64 ? 7 : 6))
    };
    static uint16_t const MAX_SIZE = (1 << MAX_LOG);
    typedef binomial_coefficients_trait<MAX_LOG> trait;
    typedef typename trait::number_type number_type;
    typedef binomial_table<MAX_SIZE, number_type> tBinom;

    static struct impl
    {
        number_type const (&table)[MAX_SIZE + 1][MAX_SIZE + 1] =
            tBinom::data.table; // table for the binomial coefficients
        uint16_t const (&jump)[MAX_SIZE + 1][MAX_SIZE] = tBinom::data.jump;
        uint16_t space[n + 1]; // for entry i,j \lceil \log( {i \choose j}+1 ) \rceil
        number_type (&L1Mask)[MAX_SIZE + 1] = tBinom::data.L1Mask;
        number_type (&O1Mask)[MAX_SIZE] = tBinom::data.O1Mask;

        impl()
        {
            static typename binomial_table<n, number_type>::impl tmp_data;
            for (int k = 0; k <= n; ++k)
            {
                space[k] = (tmp_data.table[k][n] == (number_type)1) ? 0 : trait::hi(tmp_data.table[k][n]) + 1;
            }
        }
    } data;
};

template <uint16_t n>
typename binomial_coefficients<n>::impl binomial_coefficients<n>::data;

//! Class to encode and decode binomial coefficients on the fly.
/*!
 * The basic encoding and decoding process is described in
 * Gonzalo Navarro and Eliana Providel: Fast, Small, Simple Rank/Select on Bitmaps, SEA 2012
 *
 * Implemented optimizations in the decoding process:
 *   - Constant time handling for uniform blocks (only zeros or ones in the block)
 *   - Constant time handling for blocks contains only a single one bit.
 *   - Blocks with more than n/2 set bits are decoded as their complement.
 *   - Runs of zeros are skipped with binomial_table::jump.
 *   - For operations decode_popcount, decode_select, and decode_bit a block
 *     is only decoded as long as the query is not answered yet.
 */
template <uint16_t n>
struct rrr_helper
{
    typedef binomial_coefficients<n> binomial;          //!< The struct containing the binomial coefficients
    typedef typename binomial::number_type number_type; //!< The used number type, e.g. uint64_t, uint128_t,...
    typedef typename binomial::trait trait;             //!< The number trait

    //! Returns the space usage in bits of the binary representation of the number \f${n \choose k}\f$
    static inline uint16_t space_for_bt(uint16_t i)
    {
        return binomial::data.space[i];
    }

    template <class bit_vector_type>
    static inline number_type
    decode_btnr(bit_vector_type const & bv, typename bit_vector_type::size_type btnrp, uint16_t btnrlen)
    {
        return trait::get_int(bv, btnrp, btnrlen);
    }

    template <class bit_vector_type>
    static void
    set_bt(bit_vector_type & bv, typename bit_vector_type::size_type pos, number_type bt, uint16_t space_for_bt)
    {
        trait::set_int(bv, pos, bt, space_for_bt);
    }

    template <class bit_vector_type>
    static inline uint16_t
    get_bt(bit_vector_type const & bv, typename bit_vector_type::size_type pos, uint16_t block_size)
    {
        return trait::popcount(trait::get_int(bv, pos, block_size));
    }

    static inline number_type bin_to_nr(number_type bin)
    {
        if (bin == (number_type)0 or bin == binomial::data.L1Mask[n])
        { // handle special case
            return 0;
        }
        number_type nr = 0;
        uint16_t k = trait::popcount(bin);
        uint16_t nn = n; // size of the block
        while (bin != (number_type)0)
        {
            if (1ULL & bin)
            {
                nr += binomial::data.table[k][nn - 1];
                --k; // go to the case (n-1, k-1)
            } // else go to the case (n-1, k)
            bin = (bin >> 1);
            --nn;
        }
        return nr;
    }

    //! Complement a block with more than n/2 set bits.
    /*! Complementing a block swaps its zeros and ones and reverses the order of the blocks,
     *  so the block (k, nr) becomes (n - k, {n choose k} - 1 - nr).
     *  \return Whether the block was complemented.
     */
    static inline bool complement_dense(uint16_t & k, number_type & nr)
    {
        if (k <= n / 2)
            return false;
        number_type res = binomial::data.table[k][n];
        res -= (number_type)1;
        res -= nr;
        nr = res;
        k = n - k;
        return true;
    }

    //! Find the next set bit of a block suffix.
    /*! \param m  Length of the suffix, i.e. the suffix starts at position n - m.
     *  \param k  Number of set bits in the suffix, k > 0.
     *  \param nr Number of the suffix.
     *  \return Position of the first set bit in the suffix.
     *  On return, (m, k, nr) describe the suffix after that bit.
     *  A suffix of length m holding k set bits and a number below 2^(t+1) has zeros
     *  in its first m - jump[k][t] positions, so blocks longer than 64 bits skip these
     *  for sparse suffixes and the whole block. Shorter blocks scan linearly, which is faster there.
     */
    static inline uint16_t next_one(uint16_t & m, uint16_t & k, number_type & nr)
    {
        if (k == 1)
        { // the only set bit is at position n - nr - 1
            m = static_cast<uint16_t>(static_cast<uint64_t>(nr)) + 1;
        }
        else
        {
            if (n > 64 and (k < LINEAR_DECODE_THRESHOLD or m == n))
                m = std::min(m, binomial::data.jump[k][trait::hi(nr)]);
            while (nr < binomial::data.table[k][m - 1])
                --m;
            nr -= binomial::data.table[k][m - 1];
        }
        --k;
        --m;
        return n - m - 1;
    }

    //! Suffixes with fewer set bits skip zeros with binomial_table::jump after each set bit.
    static uint16_t const LINEAR_DECODE_THRESHOLD = n / 8;

    //! Position of the sel-th set bit (t_one) or unset bit (!t_one) in the block encoded by (k, nr).
    template <bool t_one>
    static inline uint16_t select_in_block(uint16_t k, number_type nr, uint16_t sel)
    {
        uint16_t m = n;
        uint16_t ones = 0;
        while (k > 0)
        {
            uint16_t pos = next_one(m, k, nr);
            if (t_one ? ones + 1 == sel : pos - ones >= sel)
                return t_one ? pos : sel - 1 + ones;
            ++ones;
        }
        return sel - 1 + ones; // only reached for unset bits after the last set bit
    }

    //! Decode the bit at position \f$ off \f$ of the block encoded by the pair (k, nr).
    static inline bool decode_bit(uint16_t k, number_type nr, uint16_t off)
    {
        if (k == 0 or k == n)
            return k > 0;
        bool flip = complement_dense(k, nr);
        uint16_t m = n;
        while (k > 0)
        {
            uint16_t pos = next_one(m, k, nr);
            if (pos >= off)
                return (pos == off) != flip;
        }
        return flip;
    }

    //! Decode the len-bit integer starting at position \f$ off \f$ of the block encoded by the pair (k, nr).
    static inline uint64_t decode_int(uint16_t k, number_type nr, uint16_t off, uint16_t len)
    {
        if (k == 0 or k == n)
            return k > 0 ? bits::lo_set[len] : 0;
        uint64_t res = complement_dense(k, nr) ? bits::lo_set[len] : 0;
        uint16_t m = n;
        while (k > 0)
        {
            uint16_t pos = next_one(m, k, nr);
            if (pos >= off + len)
                break;
            if (pos >= off)
                res ^= 1ULL << (pos - off);
        }
        return res;
    }

    //! Decode the first off bits bits of the block encoded by the pair (k, nr) and return the set bits.
    static inline uint16_t decode_popcount(uint16_t k, number_type nr, uint16_t off)
    {
        return decode_bit_and_popcount(k, nr, off).second;
    }

    //! Decode the bit at position off and the number of set bits before it in the block encoded by (k, nr).
    /*!\pre off < n, or off == n for the number of set bits only
     */
    static inline std::pair<bool, uint16_t> decode_bit_and_popcount(uint16_t k, number_type nr, uint16_t off)
    {
        if (k == 0 or k == n)
            return {k > 0, k > 0 ? off : 0};
        bool flip = complement_dense(k, nr);
        uint16_t m = n;
        uint16_t count = 0;
        bool bit = false;
        while (k > 0)
        {
            uint16_t pos = next_one(m, k, nr);
            if (pos >= off)
            {
                bit = pos == off;
                break;
            }
            ++count;
        }
        return flip ? std::pair<bool, uint16_t>{!bit, off - count} : std::pair<bool, uint16_t>{bit, count};
    }

    //! Position of the sel-th set bit in the block encoded by (k, nr).
    /*!\pre k >= sel, sel>0
     */
    static inline uint16_t decode_select(uint16_t k, number_type nr, uint16_t sel)
    {
        if (k == n)
            return sel - 1;
        return complement_dense(k, nr) ? select_in_block<false>(k, nr, sel) : select_in_block<true>(k, nr, sel);
    }

    //! Position of the sel-th unset bit in the block encoded by (k, nr).
    /*!\pre n - k >= sel, sel>0
     */
    static inline uint16_t decode_select0(uint16_t k, number_type nr, uint16_t sel)
    {
        if (k == 0)
            return sel - 1;
        return complement_dense(k, nr) ? select_in_block<true>(k, nr, sel) : select_in_block<false>(k, nr, sel);
    }

    /*!\pre k >= sel, sel>0
     */
    template <uint8_t pattern, uint8_t len>
    static inline uint16_t decode_select_bitpattern(uint16_t k, number_type & nr, uint16_t sel)
    {
        int i = 0;
        uint8_t decoded_pattern = 0;
        uint8_t decoded_len = 0;
        uint16_t nn = n;
        while (sel > 0)
        { // TODO: this condition only work if the precondition holds
            decoded_pattern = decoded_pattern << 1;
            ++decoded_len;
            if (nr >= binomial::data.table[k][nn - 1])
            {
                nr -= binomial::data.table[k][nn - 1];
                // a one is decoded
                decoded_pattern |= 1; // add to the pattern
                --k;
            }
            --nn;
            ++i;
            if (decoded_len == len)
            { // if decoded pattern length equals len of the searched pattern
                if (decoded_pattern == pattern)
                { // and pattern equals the searched pattern
                    --sel;
                }
                decoded_pattern = 0;
                decoded_len = 0; // reset pattern
            }
        }
        return i - len; // return the starting position of $sel$th occurence of the pattern
    }
};

} // namespace sdsl
#endif
