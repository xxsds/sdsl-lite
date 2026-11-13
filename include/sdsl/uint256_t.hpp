// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
/*!\file uint256_t.hpp
 * \brief uint256_t.hpp contains a class for 256-bit unsigned integers.
 * \author Simon Gog
 */
#ifndef INCLUDED_SDSL_UINT256
#define INCLUDED_SDSL_UINT256

#include <iostream>
#include <stdint.h>
#include <concepts>
#include <type_traits>
#include <utility>

#include <sdsl/bits.hpp>
#include <sdsl/uint128_t.hpp>

namespace sdsl
{

namespace detail
{
//! Built-in integers that mix with uint256_t; libstdc++ counts uint128_t as integral only in GNU modes.
template <typename T>
concept uint256_operand = std::integral<T> || std::same_as<T, uint128_t>;
} // namespace detail

class uint256_t
{
public:
    friend std::ostream & operator<<(std::ostream &, uint256_t const &);

private:
    //! Bits 64 to 127 of a built-in integer converted modulo 2^256.
    template <typename T>
    static uint64_t mid_bits(T x)
    {
        if constexpr (std::is_same_v<T, uint128_t>)
            return uint64_t(x >> 64);
        else if constexpr (std::is_signed_v<T>)
            return x < 0 ? ~uint64_t(0) : uint64_t(0);
        else
            return 0;
    }

    //! Bits 128 to 255 of a built-in integer converted modulo 2^256.
    template <typename T>
    static uint128_t high_bits(T x)
    {
        if constexpr (std::is_signed_v<T>)
            return x < 0 ? ~uint128_t(0) : uint128_t(0);
        else
            return uint128_t(0);
    }

    uint64_t m_lo;
    uint64_t m_mid;
    uint128_t m_high;

public:
    inline uint256_t(uint64_t lo = 0, uint64_t mid = 0, uint128_t high = 0) : m_lo(lo), m_mid(mid), m_high(high)
    {}

    //! Converts any built-in integer modulo 2^256, without truncating 128-bit values.
    template <detail::uint256_operand T>
    uint256_t(T x) : m_lo(uint64_t(x)), m_mid(mid_bits(x)), m_high(high_bits(x))
    {}

    uint256_t(uint256_t const &) = default;
    uint256_t(uint256_t &&) = default;
    uint256_t & operator=(uint256_t const &) = default;
    uint256_t & operator=(uint256_t &&) = default;

    inline uint16_t popcount() const
    {
        return ((uint16_t)bits::cnt(m_lo)) + (uint16_t)bits::cnt(m_mid) + (uint16_t)bits::cnt(m_high >> 64)
             + (uint16_t)bits::cnt(m_high);
    }

    inline uint16_t hi() const
    {
        if (m_high == (uint128_t)0ULL)
        {
            if (m_mid)
            {
                return bits::hi(m_mid) + 64;
            }
            else
            {
                return bits::hi(m_lo);
            }
        }
        else
        {
            uint64_t hh = (m_high >> 64);
            if (hh)
            {
                return bits::hi(hh) + 192;
            }
            else
            {
                return bits::hi(m_high) + 128;
            }
        }
    }

    inline uint16_t select(uint32_t i) const
    {
        uint16_t x = 0;
        if ((x = (uint16_t)bits::cnt(m_lo)) >= i)
        {
            return bits::sel(m_lo, i);
        }
        i -= x;
        if ((x = (uint16_t)bits::cnt(m_mid)) >= i)
        {
            return bits::sel(m_mid, i) + 64;
        }
        i -= x;
        uint64_t hh = m_high >> 64;
        uint64_t lh = m_high;
        if ((x = (uint16_t)bits::cnt(lh)) >= i)
        {
            return bits::sel(lh, i) + 128;
        }
        i -= x;
        return bits::sel(hh, i) + 192;
    }

    inline uint256_t & operator+=(uint256_t const & x)
    {
        uint128_t lo = (uint128_t)m_lo + x.m_lo;
        uint128_t mid = (uint128_t)m_mid + x.m_mid + (lo >> 64);
        m_lo = lo;
        m_mid = mid;
        m_high += x.m_high + (mid >> 64);
        return *this;
        //			return uint256_t(lo, mid, m_high + x.m_high + (mid >> 64));
    }

    inline uint256_t operator+(uint256_t const & x) const
    {
        uint128_t lo = ((uint128_t)m_lo) + x.m_lo;
        uint128_t mid = (uint128_t)m_mid + x.m_mid + (lo >> 64);
        return uint256_t(lo, mid, m_high + x.m_high + (mid >> 64));
    }

    inline uint256_t operator-(uint256_t const & x) const
    {
        //			add two's complement of x
        uint128_t lo = (uint128_t)m_lo + (~x.m_lo) + (uint128_t)1ULL;
        uint128_t mid = (uint128_t)m_mid + (~x.m_mid) + (lo >> 64);
        return uint256_t(lo, mid, m_high + (~x.m_high) + (mid >> 64));
    }

    inline uint256_t & operator-=(uint256_t const & x)
    {
        //			add two's complement of x
        uint128_t lo = (uint128_t)m_lo + (~x.m_lo) + (uint128_t)1ULL;
        uint128_t mid = (uint128_t)m_mid + (~x.m_mid) + (lo >> 64);
        m_lo = lo;
        m_mid = mid;
        m_high += (~x.m_high) + (mid >> 64);
        return *this;
    }

    inline uint256_t operator|(uint256_t const & x) const
    {
        return uint256_t(m_lo | x.m_lo, m_mid | x.m_mid, m_high | x.m_high);
    }

    inline uint256_t & operator|=(uint256_t const & x)
    {
        m_lo |= x.m_lo;
        m_mid |= x.m_mid;
        m_high |= x.m_high;
        return *this;
    }

    inline uint256_t operator&(uint256_t const & x) const
    {
        return uint256_t(m_lo & x.m_lo, m_mid & x.m_mid, m_high & x.m_high);
    }
    /* // is not needed since we can convert uint256_t to uint64_t
     * uint64_t operator&(uint64_t x){
     * return m_lo & x;
     * }
     */

    inline uint256_t & operator&=(uint256_t const & x)
    {
        return *this = *this & x;
    }

    inline uint256_t & operator<<=(int x)
    {
        return *this = *this << x;
    }

    inline uint256_t & operator>>=(int x)
    {
        return *this = *this >> x;
    }

    inline uint256_t operator~() const
    {
        return uint256_t(~m_lo, ~m_mid, ~m_high);
    }

    template <detail::uint256_operand T>
    uint256_t operator+(T x) const
    {
        return *this + uint256_t(x);
    }

    template <detail::uint256_operand T>
    uint256_t operator-(T x) const
    {
        return *this - uint256_t(x);
    }

    template <detail::uint256_operand T>
    uint256_t operator|(T x) const
    {
        return *this | uint256_t(x);
    }

    template <detail::uint256_operand T>
    uint256_t operator&(T x) const
    {
        return *this & uint256_t(x);
    }

    template <detail::uint256_operand T>
    bool operator==(T x) const
    {
        return *this == uint256_t(x);
    }

    template <detail::uint256_operand T>
    bool operator!=(T x) const
    {
        return *this != uint256_t(x);
    }

    template <detail::uint256_operand T>
    bool operator<(T x) const
    {
        return *this < uint256_t(x);
    }

    template <detail::uint256_operand T>
    bool operator<=(T x) const
    {
        return *this <= uint256_t(x);
    }

    template <detail::uint256_operand T>
    bool operator>(T x) const
    {
        return *this > uint256_t(x);
    }

    template <detail::uint256_operand T>
    bool operator>=(T x) const
    {
        return *this >= uint256_t(x);
    }

    template <std::integral T>
    inline uint256_t operator<<(T x) const
    {
        if (x >= 128)
        {
            uint128_t high = (((uint128_t)m_mid << 64) | m_lo) << (x - 128);
            return uint256_t(0, 0, high);
        }

        // Move whole words first, then shift within each 64-bit word.
        uint64_t lo = m_lo, mid = m_mid, high_lo = m_high, high_hi = m_high >> 64;
        if (x >= 64)
        {
            high_hi = high_lo;
            high_lo = mid;
            mid = lo;
            lo = 0;
        }
        int const shift = x & 63;
        if (shift)
        {
            high_hi = (high_hi << shift) | (high_lo >> (64 - shift));
            high_lo = (high_lo << shift) | (mid >> (64 - shift));
            mid = (mid << shift) | (lo >> (64 - shift));
            lo <<= shift;
        }
        return uint256_t(lo, mid, (uint128_t(high_hi) << 64) | high_lo);
    }

    template <std::integral T>
    inline uint256_t operator>>(T x) const
    {
        if (x >= 128)
        {
            uint128_t low = m_high >> (x - 128);
            return uint256_t(low, low >> 64, 0);
        }

        uint64_t lo = m_lo, mid = m_mid, high_lo = m_high, high_hi = m_high >> 64;
        if (x >= 64)
        {
            lo = mid;
            mid = high_lo;
            high_lo = high_hi;
            high_hi = 0;
        }
        int const shift = x & 63;
        if (shift)
        {
            lo = (lo >> shift) | (mid << (64 - shift));
            mid = (mid >> shift) | (high_lo << (64 - shift));
            high_lo = (high_lo >> shift) | (high_hi << (64 - shift));
            high_hi >>= shift;
        }
        return uint256_t(lo, mid, (uint128_t(high_hi) << 64) | high_lo);
    }

    template <detail::uint256_operand T>
    uint256_t & operator=(T x)
    {
        return *this = uint256_t(x);
    }

    inline bool operator==(uint256_t const & x) const
    {
        return (m_lo == x.m_lo) and (m_mid == x.m_mid) and (m_high == x.m_high);
    }

    inline bool operator!=(uint256_t const & x) const
    {
        return !(*this == x);
    }

    inline bool operator>=(uint256_t const & x) const
    {
        if (m_high != x.m_high)
        {
            return m_high > x.m_high;
        }
        if (m_mid != x.m_mid)
        {
            return m_mid > x.m_mid;
        }
        else
        {
            return m_lo >= x.m_lo;
        }
    }

    inline bool operator<=(uint256_t const & x) const
    {
        if (m_high != x.m_high)
        {
            return m_high < x.m_high;
        }
        if (m_mid != x.m_mid)
        {
            return m_mid < x.m_mid;
        }
        else
        {
            return m_lo <= x.m_lo;
        }
    }

    inline bool operator>(uint256_t const & x) const
    {
        if (m_high != x.m_high)
        {
            return m_high > x.m_high;
        }
        if (m_mid != x.m_mid)
        {
            return m_mid > x.m_mid;
        }
        else
        {
            return m_lo > x.m_lo;
        }
    }

    inline bool operator>(uint64_t const & x) const
    {
        if (m_high > (uint128_t)0ULL or m_mid > (uint128_t)0ULL)
        {
            return true;
        }
        return m_lo > x;
    }

    inline bool operator<(uint256_t const & x) const
    {
        if (m_high != x.m_high)
        {
            return m_high < x.m_high;
        }
        if (m_mid != x.m_mid)
        {
            return m_mid < x.m_mid;
        }
        else
        {
            return m_lo < x.m_lo;
        }
    }

    explicit operator bool() const
    {
        return m_lo || m_mid || m_high;
    }

    explicit operator uint128_t() const
    {
        return (uint128_t(m_mid) << 64) | m_lo;
    }

    inline operator uint64_t() const
    {
        return m_lo;
    }
};

template <detail::uint256_operand T>
inline bool operator==(T number, uint256_t const & x)
{
    return x.operator==(number);
}

template <detail::uint256_operand T>
inline bool operator!=(T number, uint256_t const & x)
{
    return x.operator!=(number);
}

template <detail::uint256_operand T>
inline bool operator<(T number, uint256_t const & x)
{
    return x.operator>(number);
}

template <detail::uint256_operand T>
inline bool operator<=(T number, uint256_t const & x)
{
    return x.operator>=(number);
}

template <detail::uint256_operand T>
inline bool operator>(T number, uint256_t const & x)
{
    return x.operator<(number);
}

template <detail::uint256_operand T>
inline bool operator>=(T number, uint256_t const & x)
{
    return x.operator<=(number);
}

inline std::ostream & operator<<(std::ostream & os, uint256_t const & x)
{
    uint64_t X[4] = {(uint64_t)(x.m_high >> 64), (uint64_t)x.m_high, x.m_mid, x.m_lo};
    for (int j = 0; j < 4; ++j)
    {
        for (int i = 0; i < 16; ++i)
        {
            os << std::hex << ((X[j] >> 60) & 0xFULL) << std::dec;
            X[j] <<= 4;
        }
    }
    return os;
}

} // namespace sdsl

#endif
