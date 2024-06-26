// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
/*!\file coder_fibonacci.hpp
 * \brief coder_fibonacci.hpp contains the class sdsl::coder::fibonacci
 * \author Simon Gog
 */
#ifndef SDSL_CODER_FIBONACCI_INCLUDED
#define SDSL_CODER_FIBONACCI_INCLUDED

#include <stdint.h>

#include <sdsl/bits.hpp>
#include <sdsl/config.hpp>

namespace sdsl
{

namespace coder
{

//! A class to encode and decode between Fibonacci and binary code.
template <typename T = void>
class fibonacci
{
public:
    static struct impl
    {
        uint64_t fib12bit_to_bin[(1 << 12) * 8];
        //! End position of the first Fibonacci encoded number in the 13-bit word.
        /*! fib2bin_shift[x] = 0 if bit-pattern `11` does not occur in x. Otherwise
         * fib2bin_shift[x] = end position of the first Fibonacci encoded word.
         * E.g. Fib2binShift[3] = 2 and Fib2binShift[6] = 3.
         * Space: 256.0 kBytes
         */
        uint8_t fib2bin_shift[(1 << 13)];
        //! Array contains precomputed values for the decoding of a prefix sum of Fibonacci encoded integers
        /*! The 5 most significant bits contain information about how far to shift to get to the next encoded integer.
         * If this 5 bits equal zero, there is no whole Fibonacci number encoded in the 16 bits...
         * space for Fib2bin_greedy-table 128.0 kBytes
         * maxentry = 1596  index of maxentry = 54613
         */
        uint16_t fib2bin_16_greedy[(1 << 16)];

        //! Array contains precomputed values for the decoding of a number in the Fibonacci system.
        uint64_t fib2bin_0_95[(1 << 12) * 8];

        impl()
        {
            for (uint32_t x = 0; x <= 0x1FFF; ++x)
            {
                if (bits::cnt11(x))
                {
                    fib2bin_shift[x] = bits::sel11(x, 1) + 1;
                }
                else
                {
                    fib2bin_shift[x] = 0;
                }
            }
            for (uint32_t x = 0; x < 1 << 16; ++x)
            {
                uint16_t w = 0;
                uint32_t offset = 0;
                if (uint32_t cnt = bits::cnt11(x))
                {
                    uint32_t y = x;
                    uint32_t fib_pos = 1;
                    do
                    {
                        if (y & 1)
                        {
                            w += bits::lt_fib[fib_pos - 1];
                            if (y & 2)
                            {
                                --cnt;
                                ++offset;
                                fib_pos = 0;
                                y >>= 1;
                            }
                        }
                        ++fib_pos;
                        ++offset;
                        y >>= 1;
                    }
                    while (cnt);
                }
                fib2bin_16_greedy[x] = (offset << 11) | w;
            }
            for (uint32_t p = 0; p < 8; ++p)
            {
                for (uint32_t x = 0; x <= 0xFFF; ++x)
                {
                    uint64_t w = 0;
                    for (uint32_t j = 0; j < 12 and 12 * p + j < 92; ++j)
                    {
                        if ((x >> j) & 1ULL)
                        {
                            w += bits::lt_fib[12 * p + j];
                            if (x >> (j + 1) & 1ULL)
                            {
                                break;
                            }
                        }
                    }
                    fib2bin_0_95[(p << 12) | x] = w;
                }
            }
        }
    } data;

    typedef uint64_t size_type;

    static const uint8_t min_codeword_length = 2; // 11 represents 1 and is the code word with minimum length
    //! Get the number of bits that are necessary to encode the value w in Fibonacci code.
    /*!\param w 64bit integer to get the length of its fibonacci encoding. Inclusive the terminating 1 of the code.
     */
    static uint8_t encoding_length(uint64_t w);
    //! Decode n Fibonacci encoded bits beginning at start_idx in the bitstring "data"
    /* \param data Bitstring
     * \param start_idx Starting index of the decoding.
     * \param n Number of values to decode from the bitstring.
     * \param it Iterator
     */
    template <bool t_sumup, bool t_inc, class t_iter>
    static uint64_t decode(uint64_t const * data, const size_type start_idx, size_type n, t_iter it = (t_iter) nullptr);

    template <bool t_sumup, bool t_inc, class t_iter>
    static uint64_t
    decode1(uint64_t const * data, const size_type start_idx, size_type n, t_iter it = (t_iter) nullptr);

    //! Decode n Fibonacci encoded integers beginning at start_idx in the bitstring "data"  and return the sum of these
    //! values.
    /*!\param data Pointer to the beginning of the Fibonacci encoded bitstring.
     * \param start_idx Index of the first bit to encode the values from.
     * \param n Number of values to decode from the bitstring. Attention: There have to be at least n encoded values in
     * the bitstring.
     */
    static uint64_t decode_prefix_sum(uint64_t const * d, const size_type start_idx, size_type n);

    //! Decode n Fibonacci encoded integers beginning at start_idx and ending at end_idx (exclusive) in the bitstring
    //! "data" and return the sum of these values.
    /*!\sa decode_prefix_sum
     */
    static uint64_t
    decode_prefix_sum(uint64_t const * d, const size_type start_idx, const size_type end_idx, size_type n);

    template <class int_vector1, class int_vector2>
    static bool encode(int_vector1 const & v, int_vector2 & z);

    template <class int_vector>
    static uint64_t * raw_data(int_vector & v)
    {
        return v.m_data;
    }

    //! Encode one positive integer x to an int_vector at bit position start_idx.
    /*!\param x Positive integer to encode.
     * \param z Raw data of vector to write the encoded form of x.
     * \param offset Start offset to write the encoded form of x in z. \f$0\leq offset< 64\f$.
     */
    static void encode(uint64_t x, uint64_t *& z, uint8_t & offset);

    template <class int_vector1, class int_vector2>
    static bool decode(int_vector1 const & z, int_vector2 & v);
};

template <typename T>
inline uint8_t fibonacci<T>::encoding_length(uint64_t w)
{
    if (w == 0)
    {
        return 93;
    }
    // This limit for the leftmost 1bit in the resulting fib code could be improved using a table
    uint8_t len_1 = bits::hi(w); // len-1 of the fib code
    while (++len_1 < (uint8_t)(sizeof(bits::lt_fib) / sizeof(bits::lt_fib[0])) && w >= bits::lt_fib[len_1])
        ;
    return len_1 + 1;
}

template <typename T>
template <class int_vector1, class int_vector2>
inline bool fibonacci<T>::encode(int_vector1 const & v, int_vector2 & z)
{
    uint64_t z_bit_size = 0;
    uint64_t w;
    const uint64_t zero_val = v.width() < 64 ? (1ULL) << v.width() : 0;
    for (typename int_vector1::const_iterator it = v.begin(), end = v.end(); it != end; ++it)
    {
        if ((w = *it) == 0)
        {
            if (v.width() < 64)
            {
                w = zero_val;
            }
        }
        z_bit_size += encoding_length(w);
    }
    z.bit_resize(z_bit_size);
    z.shrink_to_fit();
    if (z_bit_size & 0x3F)
    {                                        // if z_bit_size % 64 != 0
        *(z.m_data + (z_bit_size >> 6)) = 0; // initialize last word
    }
    uint64_t * z_data = z.m_data;
    uint8_t offset = 0;
    uint64_t fibword_high = 0x0000000000000001ULL, fibword_low;
    uint64_t t;
    for (typename int_vector1::const_iterator it = v.begin(), end = v.end(); it != end; ++it)
    {
        w = *it;
        if (w == 0)
        {
            w = zero_val;
        }
        int8_t len_1 = encoding_length(w) - 1, j;
        fibword_low = 0x0000000000000001ULL;

        if (len_1 >= 64)
        { // length > 65
            fibword_high = 0x0000000000000001ULL;
            j = len_1 - 1;
            if (w == 0)
            { // handle special case
                fibword_high <<= 1;
                fibword_high |= 1;
                fibword_high <<= 1;
                w -= bits::lt_fib[len_1 - 1];
                j -= 2;
            }
            for (; j > 63; --j)
            {
                fibword_high <<= 1;
                if (w >= (t = bits::lt_fib[j]))
                {
                    w -= t;
                    fibword_high |= 1;
                    if (w and j > 64)
                    {
                        fibword_high <<= 1;
                        --j;
                    }
                    else
                    {
                        fibword_high <<= (64 - j);
                        break;
                    }
                }
            }
            j = 64;
        }
        else
        {
            j = len_1 - 1;
        }

        for (; j >= 0; --j)
        {
            fibword_low <<= 1;
            if (w >= (t = bits::lt_fib[j]))
            {
                w -= t;
                fibword_low |= 1;
                if (w)
                {
                    fibword_low <<= 1;
                    --j;
                }
                else
                {
                    fibword_low <<= (j);
                    break;
                }
            }
        }
        if (len_1 >= 64)
        {
            bits::write_int_and_move(z_data, fibword_low, offset, 64);
            bits::write_int_and_move(z_data, fibword_high, offset, len_1 - 63);
        }
        else
        {
            bits::write_int_and_move(z_data, fibword_low, offset, (len_1 & 0x3F) + 1);
        }
    }
    z.width(v.width());
    return true;
}

template <typename T>
inline void fibonacci<T>::encode(uint64_t x, uint64_t *& z, uint8_t & offset)
{
    uint64_t fibword_high = 0x0000000000000001ULL, fibword_low;
    uint64_t t;
    int8_t len_1 = encoding_length(x) - 1, j;
    fibword_low = 0x0000000000000001ULL;

    if (len_1 >= 64)
    { // length > 65
        fibword_high = 0x0000000000000001ULL;
        j = len_1 - 1;
        if (x == 0)
        { // handle special case
            fibword_high <<= 1;
            fibword_high |= 1;
            fibword_high <<= 1;
            x -= bits::lt_fib[len_1 - 1];
            j -= 2;
        }
        for (; j > 63; --j)
        {
            fibword_high <<= 1;
            if (x >= (t = bits::lt_fib[j]))
            {
                x -= t;
                fibword_high |= 1;
                if (x and j > 64)
                {
                    fibword_high <<= 1;
                    --j;
                }
                else
                {
                    fibword_high <<= (64 - j);
                    break;
                }
            }
        }
        j = 64;
    }
    else
    {
        j = len_1 - 1;
    }
    for (; j >= 0; --j)
    {
        fibword_low <<= 1;
        if (x >= (t = bits::lt_fib[j]))
        {
            x -= t;
            fibword_low |= 1;
            if (x)
            {
                fibword_low <<= 1;
                --j;
            }
            else
            {
                fibword_low <<= (j);
                break;
            }
        }
    }
    if (len_1 >= 64)
    {
        bits::write_int_and_move(z, fibword_low, offset, 64);
        bits::write_int_and_move(z, fibword_high, offset, len_1 - 63);
    }
    else
    {
        bits::write_int_and_move(z, fibword_low, offset, (len_1 & 0x3F) + 1);
    }
}

template <typename T>
template <class int_vector1, class int_vector2>
inline bool fibonacci<T>::decode(int_vector1 const & z, int_vector2 & v)
{
    uint64_t n = 0, carry = 0; // n = number of values to be decoded
    uint64_t const * data = z.data();
    // Determine size of v
    if (z.empty())
    { // if z is empty we are ready with decoding
        v.width(z.width());
        v.resize(0);
        v.shrink_to_fit();
        return true;
    }
    for (typename int_vector1::size_type i = 0; i < z.bit_data_size() - 1; ++i, ++data)
    {
        n += bits::cnt11(*data, carry);
    }
    if (z.bit_data_size() << 6 != z.bit_size())
    {
        n += bits::cnt11((*data) & bits::lo_set[z.bit_size() & 0x3F], carry);
    }
    else
    {
        n += bits::cnt11(*data, carry);
    }
    v.width(z.width());
    v.resize(n);
    v.shrink_to_fit();
    return decode<false, true>(z.data(), 0, n, v.begin());
}

template <typename T>
template <bool t_sumup, bool t_inc, class t_iter>
inline uint64_t fibonacci<T>::decode(uint64_t const * data, const size_type start_idx, size_type n, t_iter it)
{
    data += (start_idx >> 6);
    uint64_t w = 0, value = 0;
    int8_t buffered = 0;            // bits buffered in w, in 0..64
    int8_t read = start_idx & 0x3F; // read bits in current *data 0..63
    int8_t shift = 0;
    uint32_t fibtable = 0;
    while (n)
    { // while not all values are decoded
        while (buffered < 13 and bits::cnt11(w) < n)
        {
            w |= (((*data) >> read) << buffered);
            if (read >= buffered)
            {
                ++data;
                buffered += 64 - read;
                read = 0;
            }
            else
            { // read < buffered
                read += 64 - buffered;
                buffered = 64;
            }
        }
        value += fibonacci<T>::data.fib2bin_0_95[(fibtable << 12) | (w & 0xFFF)];
        shift = fibonacci<T>::data.fib2bin_shift[w & 0x1FFF];
        if (shift > 0)
        { // if end of decoding
            w >>= shift;
            buffered -= shift;
            if (t_inc)
                *(it++) = value;
            if (!t_sumup and n != 1)
                value = 0;
            fibtable = 0;
            --n;
        }
        else
        { // not end of decoding
            w >>= 12;
            buffered -= 12;
            ++fibtable;
        }
    }
    return value;
}

template <typename T>
template <bool t_sumup, bool t_inc, class t_iter>
inline uint64_t fibonacci<T>::decode1(uint64_t const * d, const size_type start_idx, size_type n, t_iter it)
{
    d += (start_idx >> 6);
    uint64_t w = 0, value = 0;
    int8_t buffered = 0;            // bits buffered in w, in 0..64
    int8_t read = start_idx & 0x3F; // read bits in current *data 0..63
    int8_t shift = 0;
    uint32_t fibtable = 0;
    uint8_t blocknr = (start_idx >> 6) % 9;
    while (n)
    { // while not all values are decoded
        while (buffered < 13 and bits::cnt11(w) < n)
        {
            w |= (((*d) >> read) << buffered);
            if (read >= buffered)
            {
                ++blocknr;
                ++d;
                if (blocknr == 8)
                {
                    ++d;
                    blocknr = 0;
                }
                buffered += 64 - read;
                read = 0;
            }
            else
            { // read < buffered
                read += 64 - buffered;
                buffered = 64;
            }
        }
        value += fibonacci<T>::data.fib2bin_0_95[(fibtable << 12) | (w & 0xFFF)];
        shift = fibonacci<T>::data.fib2bin_shift[w & 0x1FFF];
        if (shift > 0)
        { // if end of decoding
            w >>= shift;
            buffered -= shift;
            if (t_inc)
                *(it++) = value;
            if (!t_sumup)
                value = 0;
            fibtable = 0;
            --n;
        }
        else
        { // not end of decoding
            w >>= 12;
            buffered -= 12;
            ++fibtable;
        }
    }
    return value;
}

template <typename T>
inline uint64_t fibonacci<T>::decode_prefix_sum(uint64_t const * d, const size_type start_idx, size_type n)
{
    if (n == 0)
        return 0;
    //	return decode<true,false,int*>(data, start_idx, n);
    d += (start_idx >> 6);
    size_type i = 0;
    int32_t bits_to_decode = 0;
    uint64_t w = 0, value = 0;
    int16_t buffered = 0, read = start_idx & 0x3F, shift = 0;
    uint16_t temp = 0;
    uint64_t carry = 0;
    i = bits::cnt11(*d & ~bits::lo_set[read], carry);
    if (i < n)
    {
        uint64_t oldcarry;
        w = 0;
        do
        {
            oldcarry = carry;
            i += (temp = bits::cnt11(*(d + (++w)), carry));
        }
        while (i < n);
        bits_to_decode += ((w - 1) << 6) + bits::sel11(*(d + w), n - (i - temp), oldcarry) + 65 - read;
        w = 0;
    }
    else
    { // i>=n
        bits_to_decode = bits::sel11(*d >> read, n) + 1;
    }
    if (((size_type)bits_to_decode) == n << 1)
        return n;
    if (((size_type)bits_to_decode) == (n << 1) + 1)
        return n + 1;
    i = 0;
    //	while( bits_to_decode > 0 or buffered > 0){// while not all values are decoded
    do
    {
        while (buffered < 64 and bits_to_decode > 0)
        {
            w |= (((*d) >> read) << buffered);
            if (read >= buffered)
            {
                ++d;
                buffered += 64 - read;
                bits_to_decode -= (64 - read);
                read = 0;
            }
            else
            { // read buffered
                read += 64 - buffered;
                bits_to_decode -= (64 - buffered);
                buffered = 64;
            }
            if (bits_to_decode < 0)
            {
                buffered += bits_to_decode;
                w &= bits::lo_set[buffered];
                bits_to_decode = 0;
            }
        }
        if (!i)
        { // try do decode multiple values
            if ((w & 0xFFFFFF) == 0xFFFFFF)
            {
                value += 12;
                w >>= 24;
                buffered -= 24;
                if ((w & 0xFFFFFF) == 0xFFFFFF)
                {
                    value += 12;
                    w >>= 24;
                    buffered -= 24;
                }
            }
            do
            {
                temp = fibonacci<T>::data.fib2bin_16_greedy[w & 0xFFFF];
                if ((shift = (temp >> 11)) > 0)
                {
                    value += (temp & 0x7FFULL);
                    w >>= shift;
                    buffered -= shift;
                }
                else
                {
                    value += fibonacci<T>::data.fib2bin_0_95[w & 0xFFF];
                    w >>= 12;
                    buffered -= 12;
                    i = 1;
                    break;
                }
            }
            while (buffered > 15);
        }
        else
        { // i > 0
            value += fibonacci<T>::data.fib2bin_0_95[(i << 12) | (w & 0xFFF)];
            shift = fibonacci<T>::data.fib2bin_shift[w & 0x1FFF];
            if (shift > 0)
            { // if end of decoding
                w >>= shift;
                buffered -= shift;
                i = 0;
            }
            else
            { // not end of decoding
                w >>= 12;
                buffered -= 12;
                ++i;
            }
        }
    }
    while (bits_to_decode > 0 or buffered > 0);
    return value;
}

template <typename T>
inline uint64_t fibonacci<T>::decode_prefix_sum(uint64_t const * d,
                                                const size_type start_idx,
                                                SDSL_UNUSED const size_type end_idx,
                                                size_type n)
{
    return decode_prefix_sum(d, start_idx, n);
}

template <typename T>
typename fibonacci<T>::impl fibonacci<T>::data;

} // end namespace coder
} // end namespace sdsl
#endif
