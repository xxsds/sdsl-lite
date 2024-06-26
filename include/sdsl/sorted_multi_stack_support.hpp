// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
/*!\file sorted_multi_stack_support.hpp
 * \brief sorted_multi_stack_support.hpp contains a data structure for a stack which contains
 * elements from [0..n] in sorted order. Duplicates are possible.
 * \author Simon Gog
 */
#ifndef INCLUDED_SDSL_SORTED_MULTI_STACK_SUPPORT
#define INCLUDED_SDSL_SORTED_MULTI_STACK_SUPPORT

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

//! Stack which contains elements from [0..n] in sorted order. Duplicates are possible.
/*!\par Space complexity
 *  \f$2n\f$ bits
 */
class sorted_multi_stack_support
{
public:
    typedef int_vector<64>::size_type size_type;

private:
    size_type m_n;                      // Size of the supported vector.
    size_type m_cnt;                    // Counter for the indices on the stack.
    size_type m_top;                    // Topmost index of the stack.
    int_vector<64> m_stack;             // Memory for the stack.
    int_vector<64> m_duplication_stack; // Memory for the duplications

    inline size_type block_nr(size_type x)
    {
        return x / 63;
    }; // maybe we can speed this up with bit hacks
    inline size_type block_pos(size_type x)
    {
        return x % 63;
    }; // maybe we can speed this up with bit hacks

public:
    //! Constructor
    /*!\param n Maximum that can be pushed onto the stack
     */
    sorted_multi_stack_support(size_type n);
    sorted_multi_stack_support(sorted_multi_stack_support const &) = default;
    sorted_multi_stack_support(sorted_multi_stack_support &&) = default;
    sorted_multi_stack_support & operator=(sorted_multi_stack_support const &) = default;
    sorted_multi_stack_support & operator=(sorted_multi_stack_support &&) = default;

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
     *  \return True if there the value of the top element after the execution of pop()
     *          is not equal to the value of the top element before the execution of pop(). False otherwise.
     */
    bool pop();

    /*! Push the index x of vector vec onto the stack.
     * \par x value which should be pushed onto the stack.
     * \return True if the value on the top of the stack is smaller than x. False if the value is equal.
     * \pre top() <= x
     */
    bool push(size_type x);

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
};

inline sorted_multi_stack_support::sorted_multi_stack_support(size_type n) :
    m_n(n),
    m_cnt(0),
    m_top(0),
    m_stack(),
    m_duplication_stack()
{
    m_stack = int_vector<64>(block_nr(m_n + 1) + 1, 0);
    m_stack[0] = 1;
    m_duplication_stack = int_vector<64>((m_n >> 6) + 1, 0);
}

inline sorted_multi_stack_support::size_type sorted_multi_stack_support::top() const
{
    return m_top - 1;
}

inline bool sorted_multi_stack_support::push(size_type x)
{
    x += 1;
    size_type bn = block_nr(x);
    if (0 == ((m_stack[bn] >> block_pos(x)) & 1))
    { // check if x is not already on the stack
        m_stack[bn] ^= (1ULL << block_pos(x));
        if (bn > 0 and m_stack[bn - 1] == 0)
        {
            m_stack[bn - 1] = 0x8000000000000000ULL | m_top;
        }
        m_top = x;
        // write a 0 to the duplication stack
        // do nothing as stack is initialized with zeros
        ++m_cnt; //< increment counter
        return true;
    }
    else
    { // if the element is already on the stack
        // write a 1 to the duplication stack
        m_duplication_stack[m_cnt >> 6] ^= (1ULL << (m_cnt & 0x3F));
        ++m_cnt; //< increment counter
        return false;
    }
}

inline bool sorted_multi_stack_support::pop()
{
    if (m_cnt)
    {
        --m_cnt; //< decrement counter
        if ((m_duplication_stack[m_cnt >> 6] >> (m_cnt & 0x3F)) & 1)
        {                                                                // if it's a duplication
            m_duplication_stack[m_cnt >> 6] ^= (1ULL << (m_cnt & 0x3F)); // delete 1
            return false;
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
            return true;
        }
    }
    return false;
}

inline sorted_multi_stack_support::size_type
sorted_multi_stack_support::serialize(std::ostream & out, structure_tree_node * v, std::string name) const
{
    structure_tree_node * child = structure_tree::add_child(v, name, util::class_name(*this));
    size_type written_bytes = 0;
    written_bytes += write_member(m_n, out);
    written_bytes += write_member(m_top, out);
    written_bytes += write_member(m_cnt, out);
    written_bytes += m_stack.serialize(out);
    written_bytes += m_duplication_stack.serialize(out);
    structure_tree::add_size(child, written_bytes);
    return written_bytes;
}

inline void sorted_multi_stack_support::load(std::istream & in)
{
    read_member(m_n, in);
    read_member(m_top, in);
    read_member(m_cnt, in);
    m_stack.load(in);
    m_duplication_stack.load(in);
}

template <typename archive_t>
void sorted_multi_stack_support::CEREAL_SAVE_FUNCTION_NAME(archive_t & ar) const
{
    ar(CEREAL_NVP(m_n));
    ar(CEREAL_NVP(m_cnt));
    ar(CEREAL_NVP(m_top));
    ar(CEREAL_NVP(m_stack));
    ar(CEREAL_NVP(m_duplication_stack));
}

template <typename archive_t>
void sorted_multi_stack_support::CEREAL_LOAD_FUNCTION_NAME(archive_t & ar)
{
    ar(CEREAL_NVP(m_n));
    ar(CEREAL_NVP(m_cnt));
    ar(CEREAL_NVP(m_top));
    ar(CEREAL_NVP(m_stack));
    ar(CEREAL_NVP(m_duplication_stack));
}

} // end namespace sdsl

#endif // end file
