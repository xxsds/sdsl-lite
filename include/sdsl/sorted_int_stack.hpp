// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
/*!\file sorted_int_stack.hpp
 * \brief sorted_int_stack.hpp contains a data structure for a stack which can
 * contain numbers in strictly increasing order.
 * \author Simon Gog
 */
#ifndef INCLUDED_SDSL_SORTED_INT_STACK
#define INCLUDED_SDSL_SORTED_INT_STACK

#include <algorithm>
#include <assert.h>
#include <iosfwd>
#include <stdint.h>
#include <string>
#include <vector>

#include <sdsl/bits.hpp>
#include <sdsl/cereal.hpp>
#include <sdsl/int_vector.hpp>
#include <sdsl/io.hpp>
#include <sdsl/structure_tree.hpp>
#include <sdsl/util.hpp>

//! Namespace for the succinct data structure library.
namespace sdsl
{

//! A stack class which can contain integers in strictly increasing order.
/*!\par Space complexity
 *  \f$n+o(n)\f$ bits + 64 bits for every stored number > n-1.
 */
class sorted_int_stack
{
public:
    typedef int_vector<64>::size_type size_type;

private:
    size_type m_n;                     // maximal value which can be stored on the stack
    size_type m_cnt;                   // counter for elements on the stack
    size_type m_top;                   // top element of the stack
    int_vector<64> m_stack;            // memory for the stack
    std::vector<size_type> m_overflow; // memory for the elements which are greater than n

    inline size_type block_nr(size_type x)
    {
        return x / 63;
    }; // maybe we can speed this up with bit hacks
    inline size_type block_pos(size_type x)
    {
        return x % 63;
    }; // maybe we can speed this up with bit hacks

public:
    sorted_int_stack(size_type n);
    sorted_int_stack(sorted_int_stack const &) = default;
    sorted_int_stack(sorted_int_stack &&) = default;
    sorted_int_stack & operator=(sorted_int_stack const &) = default;
    sorted_int_stack & operator=(sorted_int_stack &&) = default;

    /*! Returns if the stack is empty.
     */
    bool empty() const
    {
        return 0 == m_cnt;
    };

    /*! Returns the topmost element of the stack.
     * \pre empty()==false
     */
    size_type top() const;

    /*! Pop the topmost element of the stack.
     */
    void pop();

    /*! Push value x on the stack.
     * \par x Value which should be pushed onto the stack.
     * \pre top() < x
     */
    void push(size_type x);

    /*! Returns the number of element is the stack.
     */
    size_type size() const
    {
        return m_cnt;
    };

    size_type serialize(std::ostream & out, structure_tree_node * v = nullptr, std::string name = "") const;
    void load(std::istream & in);
    template <typename archive_t>
    void CEREAL_SAVE_FUNCTION_NAME(archive_t & ar) const;
    template <typename archive_t>
    void CEREAL_LOAD_FUNCTION_NAME(archive_t & ar);
    bool operator==(sorted_int_stack const & other) const noexcept;
    bool operator!=(sorted_int_stack const & other) const noexcept;
};

inline sorted_int_stack::sorted_int_stack(size_type n) : m_n(n), m_cnt(0), m_top(0)
{
    m_stack = int_vector<64>(block_nr(n) + 2, 0);
    m_stack[0] = 1;
}

inline sorted_int_stack::size_type sorted_int_stack::top() const
{
    return m_top - 63;
}

inline void sorted_int_stack::push(size_type x)
{
    x += 63;
    assert(empty() || m_top < x);
    ++m_cnt; //< increment counter
    if (x > m_n + 63)
    {
        if (m_overflow.empty())
        {
            m_overflow.push_back(m_top);
        }
        m_overflow.push_back(x);
        m_top = x;
    }
    else
    {
        size_type bn = block_nr(x);
        m_stack[bn] ^= (1ULL << block_pos(x));
        if (m_stack[bn - 1] == 0)
        {
            m_stack[bn - 1] = 0x8000000000000000ULL | m_top;
        }
        m_top = x;
    }
}

inline void sorted_int_stack::pop()
{
    if (!empty())
    {
        --m_cnt; //< decrement counter
        if (m_top > m_n + 63)
        {
            m_overflow.pop_back();
            m_top = m_overflow.back();
            if (m_overflow.size() == 1)
                m_overflow.pop_back();
        }
        else
        {
            size_type bn = block_nr(m_top);
            uint64_t w = m_stack[bn];
            assert((w >> 63) == 0); // highest bit is not set, as the block contains no pointer
            w ^= (1ULL << block_pos(m_top));
            m_stack[bn] = w;
            if (w > 0)
            {
                m_top = bn * 63 + bits::hi(w);
            }
            else
            { // w==0 and cnt>0
                assert(bn > 0);
                w = m_stack[bn - 1];
                if ((w >> 63) == 0)
                { // highest bit is not set => the block contains no pointer
                    assert(w > 0);
                    m_top = (bn - 1) * 63 + bits::hi(w);
                }
                else
                { // block contains pointers
                    m_stack[bn - 1] = 0;
                    m_top = w & 0x7FFFFFFFFFFFFFFFULL;
                }
            }
        }
    }
}

inline sorted_int_stack::size_type
sorted_int_stack::serialize(std::ostream & out, structure_tree_node * v, std::string name) const
{
    structure_tree_node * child = structure_tree::add_child(v, name, util::class_name(*this));
    size_type written_bytes = 0;
    written_bytes += write_member(m_n, out);
    written_bytes += write_member(m_top, out);
    written_bytes += write_member(m_cnt, out);
    written_bytes += m_stack.serialize(out);
    written_bytes += sdsl::serialize(m_overflow, out, child, "overflow");
    structure_tree::add_size(child, written_bytes);
    return written_bytes;
}

inline void sorted_int_stack::load(std::istream & in)
{
    read_member(m_n, in);
    read_member(m_top, in);
    read_member(m_cnt, in);
    m_stack.load(in);
    sdsl::load(m_overflow, in);
}

template <typename archive_t>
void sorted_int_stack::CEREAL_SAVE_FUNCTION_NAME(archive_t & ar) const
{
    ar(CEREAL_NVP(m_n));
    ar(CEREAL_NVP(m_cnt));
    ar(CEREAL_NVP(m_top));
    ar(CEREAL_NVP(m_stack));
    ar(CEREAL_NVP(m_overflow));
}

template <typename archive_t>
void sorted_int_stack::CEREAL_LOAD_FUNCTION_NAME(archive_t & ar)
{
    ar(CEREAL_NVP(m_n));
    ar(CEREAL_NVP(m_cnt));
    ar(CEREAL_NVP(m_top));
    ar(CEREAL_NVP(m_stack));
    ar(CEREAL_NVP(m_overflow));
}

//! Equality operator.
inline bool sorted_int_stack::operator==(sorted_int_stack const & other) const noexcept
{
    return (m_n == other.m_n) && (m_cnt == other.m_cnt) && (m_top == other.m_top) && (m_stack == other.m_stack)
        && (m_overflow == other.m_overflow);
}

//! Inequality operator.
inline bool sorted_int_stack::operator!=(sorted_int_stack const & other) const noexcept
{
    return !(*this == other);
}

} // end namespace sdsl

#endif // end file
