// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
/*!\file rmq_succinct_sct.hpp
 * \brief rmq_succinct_sct.hpp contains the class rmq_succinct_sct which supports range minimum or range maximum queries
 * on a random access container in constant time and \f$2 n+o(n) bits\f$ space. \author Simon Gog
 */
#ifndef INCLUDED_SDSL_RMQ_SUCCINCT_SCT
#define INCLUDED_SDSL_RMQ_SUCCINCT_SCT

#include <assert.h>
#include <iosfwd>
#include <string>

#include <sdsl/bp_support_sada.hpp>
#include <sdsl/cereal.hpp>
#include <sdsl/int_vector.hpp>
#include <sdsl/rank_support_v5.hpp>
#include <sdsl/structure_tree.hpp>
#include <sdsl/suffix_tree_helper.hpp>
#include <sdsl/util.hpp>

//! Namespace for the succinct data structure library.
namespace sdsl
{

template <bool t_min = true, class t_bp_support = bp_support_sada<256, 32, rank_support_v5<>>>
class rmq_succinct_sct;

template <class t_bp_support = bp_support_sada<256, 32, rank_support_v5<>>>
struct range_maximum_sct
{
    typedef rmq_succinct_sct<false, t_bp_support> type;
};

//! A class to support range minimum or range maximum queries on a random access container.
/*!
 *  \tparam t_min        Specifies whether the data structure should answer range min/max queries (mimumum=true)
 *  \tparam t_bp_support Type of Support structure for the BPS-SCT.
 *
 * \par Time complexity
 *        \f$ \Order{1} \f$ for the range minimum/maximum queries if the balanced parentheses support structure supports
 * constant time operations. \par Space complexity: \f$ \Order{2n}+o(n) \f$ bits for the data structure ( \f$ n=size()
 * \f$ ).
 */
template <bool t_min, class t_bp_support>
class rmq_succinct_sct
{
    bit_vector m_sct_bp;           //!< A bit vector which contains the BPS-SCT of the input container.
    t_bp_support m_sct_bp_support; //!< Support structure for the BPS-SCT

public:
    typedef typename bit_vector::size_type size_type;
    typedef typename bit_vector::size_type value_type;
    typedef t_bp_support bp_support_type;

    bit_vector const & sct_bp = m_sct_bp;
    bp_support_type const & sct_bp_support = m_sct_bp_support;

    //! Default constructor
    rmq_succinct_sct()
    {}

    //! Constructor
    /*!\tparam t_rac A random access container.
     *  \param  v     Pointer to container object.
     */
    template <class t_rac>
    rmq_succinct_sct(t_rac const * v = nullptr)
    {
        if (v != nullptr)
        {
#ifdef RMQ_SCT_BUILD_BP_NOT_SUCCINCT
            // this method takes \f$n\log n\f$ bits extra space in the worst case
            construct_supercartesian_tree_bp(*v, m_sct_bp, t_min);
#else
            // this method takes only \f$n\f$ bits extra space in all cases
            m_sct_bp = construct_supercartesian_tree_bp_succinct(*v, t_min);
//  TODO: constructor which uses int_vector_buffer
#endif
            m_sct_bp_support = bp_support_type(&m_sct_bp);
        }
    }

    //! Copy constructor
    rmq_succinct_sct(rmq_succinct_sct const & rm) : m_sct_bp(rm.m_sct_bp), m_sct_bp_support(rm.m_sct_bp_support)
    {
        m_sct_bp_support.set_vector(&m_sct_bp);
    }

    //! Move constructor
    rmq_succinct_sct(rmq_succinct_sct && rm)
    {
        *this = std::move(rm);
    }

    rmq_succinct_sct & operator=(rmq_succinct_sct const & rm)
    {
        if (this != &rm)
        {
            rmq_succinct_sct tmp(rm);
            *this = std::move(tmp);
        }
        return *this;
    }

    rmq_succinct_sct & operator=(rmq_succinct_sct && rm)
    {
        if (this != &rm)
        {
            m_sct_bp = std::move(rm.m_sct_bp);
            m_sct_bp_support = std::move(rm.m_sct_bp_support);
            m_sct_bp_support.set_vector(&m_sct_bp);
        }
        return *this;
    }

    //! Range minimum/maximum query for the supported random access container v.
    /*!
     * \param l Leftmost position of the interval \f$[\ell..r]\f$.
     * \param r Rightmost position of the interval \f$[\ell..r]\f$.
     * \return The minimal index i with \f$\ell \leq i \leq r\f$ for which \f$ v[i] \f$ is minimal/maximal.
     * \pre
     *   - r < size()
     *   - \f$ \ell \leq r \f$
     * \par Time complexity
     *      \f$ \Order{1} \f$
     */
    size_type operator()(const size_type l, const size_type r) const
    {
        assert(l <= r);
        assert(r < size());
        if (l == r)
            return l;
        size_type i = m_sct_bp_support.select(l + 1);
        size_type j = m_sct_bp_support.select(r + 1);
        size_type fc_i = m_sct_bp_support.find_close(i);
        if (j < fc_i)
        { // i < j < find_close(j) < find_close(i)
            return l;
        }
        else
        { // if i < find_close(i) < j < find_close(j)
            size_type ec = m_sct_bp_support.rr_enclose(i, j);
            if (ec == m_sct_bp_support.size())
            { // no restricted enclosing pair found
                return r;
            }
            else
            {                                         // found range restricted enclosing pair
                return m_sct_bp_support.rank(ec) - 1; // subtract 1, as the index is 0 based
            }
        }
    }

    size_type size() const
    {
        return m_sct_bp.size() / 2;
    }

    size_type serialize(std::ostream & out, structure_tree_node * v = nullptr, std::string name = "") const
    {
        structure_tree_node * child = structure_tree::add_child(v, name, util::class_name(*this));
        size_type written_bytes = 0;
        written_bytes += m_sct_bp.serialize(out, child, "sct_bp");
        written_bytes += m_sct_bp_support.serialize(out, child, "sct_bp_support");
        structure_tree::add_size(child, written_bytes);
        return written_bytes;
    }

    void load(std::istream & in)
    {
        m_sct_bp.load(in);
        m_sct_bp_support.load(in, &m_sct_bp);
    }

    template <typename archive_t>
    void CEREAL_SAVE_FUNCTION_NAME(archive_t & ar) const
    {
        ar(CEREAL_NVP(m_sct_bp));
        ar(CEREAL_NVP(m_sct_bp_support));
    }

    template <typename archive_t>
    void CEREAL_LOAD_FUNCTION_NAME(archive_t & ar)
    {
        ar(CEREAL_NVP(m_sct_bp));
        ar(CEREAL_NVP(m_sct_bp_support));
        m_sct_bp_support.set_vector(&m_sct_bp);
    }

    //! Equality operator.
    bool operator==(rmq_succinct_sct const & other) const noexcept
    {
        return (m_sct_bp == other.m_sct_bp) && (m_sct_bp_support == other.m_sct_bp_support);
    }

    //! Inequality operator.
    bool operator!=(rmq_succinct_sct const & other) const noexcept
    {
        return !(*this == other);
    }
};

} // end namespace sdsl
#endif
