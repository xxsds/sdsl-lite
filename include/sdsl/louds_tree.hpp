// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
/*!\file louds_tree.hpp
 * \brief louds_tree.hpp contains a classes for the succinct tree representation LOUDS (level order unary degree
 * sequence). \author Simon Gog
 */
#ifndef INCLUDED_SDSL_LOUDS_TREE
#define INCLUDED_SDSL_LOUDS_TREE

#include <ostream>
#include <string>
#include <utility>

#include <sdsl/cereal.hpp>
#include <sdsl/int_vector.hpp>
#include <sdsl/structure_tree.hpp>
#include <sdsl/util.hpp>

//! Namespace for the succinct data structure library.
namespace sdsl
{

//! A class for the node representation of louds_tree
class louds_node
{
public:
    typedef bit_vector::size_type size_type;

private:
    size_type m_nr;  // node number
    size_type m_pos; // position in the LOUDS

public:
    size_type const & nr;
    size_type const & pos;

    louds_node(size_type f_nr = 0, size_type f_pos = 0) : m_nr(f_nr), m_pos(f_pos), nr(m_nr), pos(m_pos)
    {}

    bool operator==(louds_node const & v) const
    {
        return m_nr == v.m_nr and m_pos == v.m_pos;
    }

    bool operator!=(louds_node const & v) const
    {
        return !(v == *this);
    }
};

inline std::ostream & operator<<(std::ostream & os, louds_node const & v)
{
    os << "(" << v.nr << "," << v.pos << ")";
    return os;
}

//! A tree class based on the level order unary degree sequence (LOUDS) representation.
/*!
 * \tparam bit_vec_t  The bit vector representation used for LOUDS.
 * \tparam select_1_t A select_support on 1-bits required for the child(v,i) operation.
 * \tparam select_0_t A select_support on 0-bits required for the parent operation.
 *
 * Example of the structure: A tree with balanced parentheses representation (()()(()()))
 * is translated into 10001110011. Traverse the tree in breadth-first order an write
 * for each node a 1-bit followed by as many 0-bits as the node has children.
 *
 * Disadvantages of louds: No efficient support for subtree size.
 */
template <class bit_vec_t = bit_vector,
          class select_1_t = typename bit_vec_t::select_1_type,
          class select_0_t = typename bit_vec_t::select_0_type>
class louds_tree
{
public:
    typedef bit_vector::size_type size_type;
    typedef louds_node node_type;
    typedef bit_vec_t bit_vector_type;
    typedef select_1_t select_1_type;
    typedef select_0_t select_0_type;

private:
    bit_vector_type m_bv;       // bit vector for the LOUDS sequence
    select_1_type m_bv_select1; // select support for 1-bits on m_bv
    select_0_type m_bv_select0; // select support for 0-bits on m_bv

public:
    bit_vector_type const & bv; // const reference to the LOUDS sequence

    //! Constructor for a cst and a root node for the traversal
    template <class Cst, class CstBfsIterator>
    louds_tree(Cst const & cst, const CstBfsIterator begin, const CstBfsIterator end) :
        m_bv(),
        m_bv_select1(),
        m_bv_select0(),
        bv(m_bv)
    {
        bit_vector tmp_bv(4 * cst.size(*begin), 0); // resize the bit_vector to the maximal
        // possible size 2*2*#leaves in the tree
        size_type pos = 0;
        for (CstBfsIterator it = begin; it != end;)
        {
            tmp_bv[pos++] = 1;
            size_type size = it.size();
            ++it;
            pos += it.size() + 1 - size;
        }
        tmp_bv.resize(pos);
        tmp_bv.shrink_to_fit();
        m_bv = bit_vector_type(std::move(tmp_bv));
        util::init_support(m_bv_select1, &m_bv);
        util::init_support(m_bv_select0, &m_bv);
    }

    louds_tree(louds_tree const & lt) :
        m_bv(lt.m_bv),
        m_bv_select1(lt.m_bv_select1),
        m_bv_select0(lt.m_bv_select0),
        bv(m_bv)
    {
        m_bv_select1.set_vector(&m_bv);
        m_bv_select0.set_vector(&m_bv);
    }

    louds_tree(louds_tree && lt) : bv(m_bv)
    {
        *this = std::move(lt);
    }

    louds_tree & operator=(louds_tree const & lt)
    {
        if (this != &lt)
        {
            louds_tree tmp(lt);
            *this = std::move(tmp);
        }
        return *this;
    }

    louds_tree & operator=(louds_tree && lt)
    {
        if (this != &lt)
        {
            m_bv = std::move(lt.m_bv);
            m_bv_select1 = std::move(lt.m_bv_select1);
            m_bv_select1.set_vector(&m_bv);
            m_bv_select0 = std::move(lt.m_bv_select0);
            m_bv_select0.set_vector(&m_bv);
        }
        return *this;
    }

    //! Returns the root node
    node_type root() const
    {
        return louds_node(0, 0);
    }

    //! Returns the number of nodes in the tree.
    size_type nodes() const
    {
        return (m_bv.size() + 1) / 2;
    }

    //! Indicates if a node is a leaf.
    /*!\param v A node.
     */
    bool is_leaf(node_type const & v) const
    {
        // node is the last leaf        or  has no children, so m_bv[v.pos]==1
        return (v.pos + 1 == m_bv.size()) or m_bv[v.pos + 1];
    }

    //! Returns the number of children of a node.
    /*!
     *  \param v A node.
     */
    size_type degree(node_type const & v) const
    {
        if (is_leaf(v))
        { // handles boundary cases
            return 0;
        }
        // position of the next node  - node position - 1
        return m_bv_select1(v.nr + 2) - v.pos - 1;
    }

    //! Returns the i-child of a node.
    /*!
     * \param v    The parent node.
     * \param i Index of the child. Indexing starts at 1.
     * \pre \f$ i \in [1..degree(v)] \f$
     */
    node_type child(node_type const & v, size_type i) const
    {
        size_type pos = v.pos + i; // go to the position of the child's zero
        // (#bits = pos+1) - (#1-bits = v.nr+1)
        size_type zeros = pos + 1 - (v.nr + 1);
        return louds_node(zeros, m_bv_select1(zeros + 1));
    }

    //! Returns the parent of a node v or root() if v==root().
    node_type parent(node_type const & v) const
    {
        if (v == root())
        {
            return root();
        }
        size_type zero_pos = m_bv_select0(v.nr);
        size_type parent_nr = (zero_pos + 1) - v.nr - 1;
        return node_type(parent_nr, m_bv_select1(parent_nr + 1));
    }

    //! Returns an unique id for each node in [0..size()-1]
    size_type id(node_type const & v) const
    {
        return v.nr;
    }

    size_type serialize(std::ostream & out, structure_tree_node * v = nullptr, std::string name = "") const
    {
        structure_tree_node * child = structure_tree::add_child(v, name, util::class_name(*this));
        size_type written_bytes = 0;
        written_bytes += m_bv.serialize(out, child, "bitvector");
        written_bytes += m_bv_select1.serialize(out, child, "select1");
        written_bytes += m_bv_select0.serialize(out, child, "select0");
        structure_tree::add_size(child, written_bytes);
        return written_bytes;
    }

    void load(std::istream & in)
    {
        m_bv.load(in);
        m_bv_select1.load(in);
        m_bv_select1.set_vector(&m_bv);
        m_bv_select0.load(in);
        m_bv_select0.set_vector(&m_bv);
    }

    //!\brief Serialise (save) via cereal
    template <typename archive_t>
    void CEREAL_SAVE_FUNCTION_NAME(archive_t & ar) const
    {
        ar(CEREAL_NVP(m_bv));
        ar(CEREAL_NVP(m_bv_select1));
        ar(CEREAL_NVP(m_bv_select0));
    }

    //!\brief Load via cereal
    template <typename archive_t>
    void CEREAL_LOAD_FUNCTION_NAME(archive_t & ar)
    {
        ar(CEREAL_NVP(m_bv));
        ar(CEREAL_NVP(m_bv_select1));
        m_bv_select1.set_vector(&m_bv);
        ar(CEREAL_NVP(m_bv_select0));
        m_bv_select0.set_vector(&m_bv);
    }
};

} // end namespace sdsl
#endif
