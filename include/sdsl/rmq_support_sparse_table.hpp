// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
/*! \file rmq_support_sparse_table.hpp
    \brief rmq_support_sparse_table.hpp contains the class rmq_support_sparse_table.
    \author Simon Gog
*/
#ifndef INCLUDED_SDSL_RMQ_SUPPORT_SPARSE_TABLE
#define INCLUDED_SDSL_RMQ_SUPPORT_SPARSE_TABLE

#include "rmq_support.hpp"
#include "int_vector.hpp"
#include <ostream>

//! Namespace for the succinct data structure library.
namespace sdsl {


template <class t_rac = int_vector<>, bool t_min = true>
class rmq_support_sparse_table;

template <class t_rac					 = int_vector<>>
using range_maximum_support_sparse_table = rmq_support_sparse_table<t_rac, false>;


//! A class to support range minimum or range maximum queries on a random access container.
/*!
 * \tparam t_rac Type of random access container for which the structure should be build.
 * \tparam t_min        Specifies whether the data structure should answer range min/max queries (mimumum=true)
 *
 * \par Reference
 *      Michael A. Bender, Martin Farach-Colton:
 *      The LCA Problem Revisited.
 *      LATIN 2000: 88-94
 *
 * \par Time complexity
 *        \f$ \Order{1} \f$ for the range minimum/maximum queries.
 * \par Space complexity:
 *      \f$ \Order{n\log^2 n} \f$ bits for the data structure ( \f$ n=size() \f$ ).
 *       We used bit compression to get a good result in practice.
 */
template <class t_rac, bool t_min>
class rmq_support_sparse_table {
	const t_rac*			  m_v; // pointer to the supported random access container
	bit_vector::size_type	 m_k; // size of m_table
	std::vector<int_vector<>> m_table;
	typedef min_max_trait<t_rac, t_min> mm_trait;


public:
	typedef typename t_rac::size_type size_type;
	typedef typename t_rac::size_type value_type;

	rmq_support_sparse_table(const t_rac* v = nullptr) : m_v(v), m_k(0)
	{
		if (m_v == nullptr) return;
		const size_type n = m_v->size();
		if (n < 2) // for n<2 the queries could be answerd without any table
			return;
		size_type k = 0;
		while (2 * (1ULL << k) < n)
			++k; // calculate maximal
		m_table.resize(k);
		m_k = k;
		for (size_type i = 0; i < k; ++i) {
			m_table[i] = int_vector<>(n - (1ULL << (i + 1)) + 1, 0, i + 1);
		}
		for (size_type i = 0; i < n - 1; ++i) {
			if (!mm_trait::compare((*m_v)[i], (*m_v)[i + 1])) m_table[0][i] = 1;
		}
		for (size_type i = 1; i < k; ++i) {
			for (size_type j = 0; j < m_table[i].size(); ++j) {
				m_table[i][j] =
				mm_trait::compare((*m_v)[j + m_table[i - 1][j]],
								  (*m_v)[j + (1ULL << i) + m_table[i - 1][j + (1ULL << i)]]) ?
				m_table[i - 1][j] :
				(1ULL << i) + m_table[i - 1][j + (1ULL << i)];
			}
		}
	}

	//! Copy constructor
	rmq_support_sparse_table(const rmq_support_sparse_table& rm) = default;

	//! Move constructor
	rmq_support_sparse_table(rmq_support_sparse_table&& rm) = default;

	rmq_support_sparse_table& operator=(const rmq_support_sparse_table& rm) = default;

	rmq_support_sparse_table& operator=(rmq_support_sparse_table&& rm) = default;

	void set_vector(const t_rac* v) { m_v = v; }

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
		if (l == r) return l;
		if (l + 1 == r) return mm_trait::compare((*m_v)[l], (*m_v)[r]) ? l : r;
		size_type		k  = bits::hi(r - l);
		const size_type rr = r - (1ULL << k) + 1;
		return mm_trait::compare((*m_v)[l + m_table[k - 1][l]], (*m_v)[rr + m_table[k - 1][rr]]) ?
			   l + m_table[k - 1][l] :
			   rr + m_table[k - 1][rr];
	}

	size_type size() const
	{
		if (m_v == nullptr)
			return 0;
		else
			return m_v->size();
	}

	size_type
	serialize(std::ostream& out, structure_tree_node* v = nullptr, std::string name = "") const
	{
		size_type			 written_bytes = 0;
		structure_tree_node* child = structure_tree::add_child(v, name, util::class_name(*this));
		written_bytes += write_member(m_k, out);
		if (m_k > 0) {
			for (size_type i = 0; i < m_k; ++i)
				written_bytes += m_table[i].serialize(out);
		}
		structure_tree::add_size(child, written_bytes);
		return written_bytes;
	}

	void load(std::istream& in, const t_rac* v)
	{
		set_vector(v);
		read_member(m_k, in);
		if (m_k > 0) {
			m_table.resize(m_k);
			for (size_type i = 0; i < m_k; ++i)
				m_table[i].load(in);
		}
	}

	template <typename archive_t>
	void CEREAL_SAVE_FUNCTION_NAME(archive_t & ar) const
	{
		ar(CEREAL_NVP(m_k));
		for (size_type i = 0; i < m_k; ++i) {
			ar(CEREAL_NVP(m_table[i]));
		}
	}

	template <typename archive_t>
	void CEREAL_LOAD_FUNCTION_NAME(archive_t & ar)
	{
		ar(CEREAL_NVP(m_k));
		m_table.resize(m_k);
		for (size_type i = 0; i < m_k; ++i) {
			ar(CEREAL_NVP(m_table[i]));
	    	}
	}

	//! Equality operator.
	bool operator==(rmq_support_sparse_table const & other) const noexcept
	{
		return (m_table == other.m_table);
	}

	//! Inequality operator.
	bool operator!=(rmq_support_sparse_table const & other) const noexcept
	{
		return !(*this == other);
	}
};

} // end namespace sds;
#endif
