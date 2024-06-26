// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
/*!\file sorted_stack_support.hpp
 * \author Simon Gog
 */
#ifndef INCLUDED_SDSL_SORTED_STACK_SUPPORT
#define INCLUDED_SDSL_SORTED_STACK_SUPPORT

#include <assert.h>
#include <iosfwd>
#include <stdint.h>
#include <string>

#include <sdsl/bits.hpp>
#include <sdsl/cereal.hpp>
#include <sdsl/int_vector.hpp>
#include <sdsl/io.hpp>
#include <sdsl/structure_tree.hpp>
#include <sdsl/util.hpp>

namespace sdsl
{
//! A stack which contains strictly increasing numbers in the range from \f$0\f$ to \f$n\f$.
/*!
 *  \par Reference
 *  Johannes Fischer:
 *  Optimal Succinctness for Range Minimum Queries
 *  LATIN 2010
 *
 *  \par Space complexity
 *    \f$n\f$ bits
 */
class sorted_stack_support
{
public:
    typedef int_vector<64>::size_type size_type;

private:
    size_type m_n;          // Size of the supported vector.
    size_type m_cnt;        // Counter for the indices on the stack.
    size_type m_top;        // Topmost index of the stack.
    int_vector<64> m_stack; // Memory for the stack.

    inline size_type block_nr(size_type x)
    {
        return x / 63;
    }; // TODO: maybe we can speed this up with bit hacks
    inline size_type block_pos(size_type x)
    {
        return x % 63;
    }; // TODO: maybe we can speed this up with bit hacks

public:
    //! Constructor
    /*!\param n Maximum that can be pushed onto the stack
     */
    sorted_stack_support(size_type n);

    sorted_stack_support(sorted_stack_support const &) = default;
    sorted_stack_support(sorted_stack_support &&) = default;
    sorted_stack_support & operator=(sorted_stack_support const &) = default;
    sorted_stack_support & operator=(sorted_stack_support &&) = default;

    /*! Returns if the stack is empty.
     */
    bool empty() const
    {
        return 0 == m_cnt;
    };

    /*! Returns the topmost index on the stack.
     * \pre empty()==false
     */
    size_type top() const;

    /*! Pop the topmost index of the stack.
     */
    void pop();

    /*! Push the index x of vector vec onto the stack.
     * \par x Index of the value in vec which should be pushed onto the stack.
     * \pre top() < x and x <= n
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
    bool operator==(sorted_stack_support const & other) const noexcept;
    bool operator!=(sorted_stack_support const & other) const noexcept;
};

inline sorted_stack_support::sorted_stack_support(size_type n) : m_n(n), m_cnt(0), m_top(0), m_stack()
{
    m_stack = int_vector<64>(block_nr(m_n + 1) + 1, 0);
    m_stack[0] = 1;
}

inline sorted_stack_support::size_type sorted_stack_support::top() const
{
    assert(empty() == false);
    return m_top - 1;
}

inline void sorted_stack_support::push(size_type x)
{
    assert((empty() or top() < x) and x <= m_n);
    x += 1;
    ++m_cnt; //< increment counter
    size_type bn = block_nr(x);
    m_stack[bn] ^= (1ULL << block_pos(x));
    if (bn > 0 and m_stack[bn - 1] == 0)
    {
        m_stack[bn - 1] = 0x8000000000000000ULL | m_top;
    }
    m_top = x;
}

inline void sorted_stack_support::pop()
{
    if (!empty())
    {
        --m_cnt; //< decrement counter
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

inline sorted_stack_support::size_type
sorted_stack_support::serialize(std::ostream & out, structure_tree_node * v, std::string name) const
{
    structure_tree_node * child = structure_tree::add_child(v, name, util::class_name(*this));
    size_type written_bytes = 0;
    written_bytes += write_member(m_n, out);
    written_bytes += write_member(m_top, out);
    written_bytes += write_member(m_cnt, out);
    written_bytes += m_stack.serialize(out);
    structure_tree::add_size(child, written_bytes);
    return written_bytes;
}

inline void sorted_stack_support::load(std::istream & in)
{
    read_member(m_n, in);
    read_member(m_top, in);
    read_member(m_cnt, in);
    m_stack.load(in);
}

template <typename archive_t>
void sorted_stack_support::CEREAL_SAVE_FUNCTION_NAME(archive_t & ar) const
{
    ar(CEREAL_NVP(m_n));
    ar(CEREAL_NVP(m_cnt));
    ar(CEREAL_NVP(m_top));
    ar(CEREAL_NVP(m_stack));
}

template <typename archive_t>
void sorted_stack_support::CEREAL_LOAD_FUNCTION_NAME(archive_t & ar)
{
    ar(CEREAL_NVP(m_n));
    ar(CEREAL_NVP(m_cnt));
    ar(CEREAL_NVP(m_top));
    ar(CEREAL_NVP(m_stack));
}

//! Equality operator.
inline bool sorted_stack_support::operator==(sorted_stack_support const & other) const noexcept
{
    return (m_n == other.m_n) && (m_cnt == other.m_cnt) && (m_top == other.m_top) && (m_stack == other.m_stack);
}

//! Inequality operator.
inline bool sorted_stack_support::operator!=(sorted_stack_support const & other) const noexcept
{
    return !(*this == other);
}

} // end namespace sdsl

#endif // end file
