// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
/*! \file wt_rlmn.hpp
    \brief wt_rlmn.hpp contains a class for a compressed wavelet tree.
	  Compression is achieved by exploiting runs in the input sequence.
    \author Simon Gog
*/
#ifndef INCLUDED_SDSL_WT_RLMN
#define INCLUDED_SDSL_WT_RLMN

#include "sdsl_concepts.hpp"
#include "int_vector.hpp"
#include "sd_vector.hpp" // for standard initialisation of template parameters
#include "util.hpp"
#include "wt_huff.hpp"
#include <algorithm> // for std::swap
#include <stdexcept>
#include <vector>
#include <utility> // for pair
#include <queue>
#include <iostream>

//! Namespace for the succinct data structure library.
namespace sdsl {

template <class t_alphabet_cat>
struct wt_rlmn_trait {
	enum { width = 0 };
	typedef int_vector<> C_type;
	typedef int_vector<> C_bf_rank_type;

	static std::map<uint64_t, uint64_t> temp_C() { return std::map<uint64_t, uint64_t>(); }

	static C_type init_C(std::map<uint64_t, uint64_t>& C, uint64_t size)
	{
		uint64_t max_symbol = (--C.end())->first;
		return C_type(max_symbol + 1, 0, bits::hi(size) + 1);
	}

	static C_bf_rank_type init_C_bf_rank(const C_type& C, uint64_t size)
	{
		return C_bf_rank_type(C.size(), 0, bits::hi(size) + 1);
	}
};

template <>
struct wt_rlmn_trait<byte_alphabet_tag> {
	enum { width = 8 };
	typedef int_vector<64> C_type;
	typedef int_vector<64> C_bf_rank_type;

	static int_vector<64> temp_C() { return int_vector<64>(256, 0); }

	static C_type init_C(C_type& C, uint64_t) { return C; }

	static C_bf_rank_type init_C_bf_rank(const C_type&, uint64_t) { return int_vector<64>(256, 0); }
};

//! A Wavelet Tree class for byte sequences.
/*!
 *    \par Space complexity
 *         \f$ nH_0 + 2|\Sigma|\log n + 2n + o(n) \f$ bits, where \f$n\f$
 *          is the size of the vector the wavelet tree was build for.
 *
 *   @ingroup wt
 *
 *  \tparam t_bitvector Type of the bitvector which is used to represent bf and
 *                      bl which mark the head of each run in the original
 *                      sequence.
 *  \tparam t_rank      Type of the rank support for bitvectors bf and bl.
 *  \tparam t_select    Type of the select support for bitvectors bf and lb.
 *  \tparam t_wt        Type of the wavelet tree for the string consisting of
 *                      the heads of the runs of the original sequence.
 *  \par Reference:
 *     Veli Mäkinen, Gonzalo Navarro:
 *     Succinct Suffix Arrays Based on Run-Length Encoding.
 *     CPM 2005: 45-56
 */
template <class t_bitvector = sd_vector<>,
		  class t_rank		= typename t_bitvector::rank_1_type,
		  class t_select	= typename t_bitvector::select_1_type,
		  class t_wt		= wt_huff<>>
class wt_rlmn {
public:
	typedef t_wt								  wt_type;
	typedef int_vector<>::size_type				  size_type;
	typedef typename t_wt::value_type			  value_type;
	typedef typename t_bitvector::difference_type difference_type;
	typedef random_access_const_iterator<wt_rlmn> const_iterator;
	typedef const_iterator						  iterator;
	typedef t_bitvector							  bit_vector_type;
	typedef t_rank								  rank_support_type;
	typedef t_select							  select_support_type;
	typedef wt_tag								  index_category;
	typedef typename t_wt::alphabet_category	  alphabet_category;
	enum { lex_ordered = false }; // TODO: is should be possible
	enum { width = wt_rlmn_trait<alphabet_category>::width };
	typedef typename wt_rlmn_trait<alphabet_category>::C_type		  C_type;
	typedef typename wt_rlmn_trait<alphabet_category>::C_bf_rank_type C_bf_rank_type;
	// to support all lex_ordered
	// operations if t_wt::lex_ordered is
	// true

private:
	size_type		m_size = 0; // size of the original input sequence
	bit_vector_type m_bl;		// bit vector for starts of runs in
	// the BWT (or last column), i.e. _b_ _l_ast
	bit_vector_type m_bf; // bit vector for starts of runs in
	// the first column of the sorted suffixes, i.e _b_ _f_irst
	wt_type m_wt; // wavelet tree for all levels
	// two equal chars
	rank_support_type   m_bl_rank;   // rank support for bit vector bl
	rank_support_type   m_bf_rank;   // rank support for bit vector bf
	select_support_type m_bl_select; // select support for bit vector bl
	select_support_type m_bf_select; // select support for bit vector bf
	C_type				m_C;		 //
	C_bf_rank_type		m_C_bf_rank; // stores the number of 1s in m_bf for
	// the prefixes m_bf[0..m_C[0]],m_bf[0..m_C[1]],....,m_bf[0..m_C[255]];
	// named C_s in the original paper

public:
	const size_type& sigma = m_wt.sigma;

	// Default constructor
	wt_rlmn() = default;

	//! Construct the wavelet tree from a sequence defined by two interators
	/*! 
         * \param begin   Iterator to the start of the input.
         * \param end     Iterator one past the end of the input.
         * \param tmp_dir Temporary directory for intermediate results.
         */
	template <typename t_it>
	wt_rlmn(t_it begin, t_it end, std::string tmp_dir = ram_file_name(""))
		: m_size(std::distance(begin, end))
	{
		std::string temp_file =
		tmp_dir + +"_wt_rlmn_" + util::to_string(util::pid()) + "_" + util::to_string(util::id());
		{
			if (0 == m_size) return;
			int_vector_buffer<width> condensed_wt(temp_file, std::ios::out);
			// scope for bl and bf
			bit_vector bl = bit_vector(m_size, 0);

			auto	   C	  = wt_rlmn_trait<alphabet_category>::temp_C();
			value_type last_c = (value_type)0;
			size_type  j	  = 0;
			for (auto it = begin; it != end; ++it, ++j) {
				value_type c = *it;
				if (last_c != c or it == begin) {
					bl[j] = 1;
					condensed_wt.push_back(c);
				}
				++C[c];
				last_c = c;
			}
			condensed_wt.close();
			m_C = wt_rlmn_trait<alphabet_category>::init_C(C, m_size);

			for (size_type i = 0, prefix_sum = 0; i < m_C.size(); ++i) {
				m_C[i] = prefix_sum;
				prefix_sum += C[i];
			}

			C_type	 lf_map = m_C;
			bit_vector bf	 = bit_vector(m_size + 1, 0);
			bf[m_size]		  = 1; // initialize last element
			j				  = 0;
			for (auto it = begin; it != end; ++it, ++j) {
				value_type c = *it;
				if (bl[j]) {
					bf[lf_map[c]] = 1;
				}
				++lf_map[c];
			}
			{
				int_vector_buffer<width> temp_bwt_buf(temp_file);
				m_wt = wt_type(temp_bwt_buf.begin(), temp_bwt_buf.end(), tmp_dir);
			}
			sdsl::remove(temp_file);
			m_bl = bit_vector_type(std::move(bl));
			m_bf = bit_vector_type(std::move(bf));
		}

		util::init_support(m_bl_rank, &m_bl);
		util::init_support(m_bf_rank, &m_bf);
		util::init_support(m_bf_select, &m_bf);
		util::init_support(m_bl_select, &m_bl);

		m_C_bf_rank = wt_rlmn_trait<alphabet_category>::init_C_bf_rank(m_C, m_size);
		for (size_type i = 0; i < m_C.size(); ++i) {
			m_C_bf_rank[i] = m_bf_rank(m_C[i]);
		}
	}

	//! Copy constructor
	wt_rlmn(const wt_rlmn& wt)
		: m_size(wt.m_size)
		, m_bl(wt.m_bl)
		, m_bf(wt.m_bf)
		, m_wt(wt.m_wt)
		, m_bl_rank(wt.m_bl_rank)
		, m_bf_rank(wt.m_bf_rank)
		, m_bl_select(wt.m_bl_select)
		, m_bf_select(wt.m_bf_select)
		, m_C(wt.m_C)
		, m_C_bf_rank(wt.m_C_bf_rank)
	{
		m_bl_rank.set_vector(&m_bl);
		m_bf_rank.set_vector(&m_bf);
		m_bl_select.set_vector(&m_bl);
		m_bf_select.set_vector(&m_bf);
	}

	//! Move constructor
	wt_rlmn(wt_rlmn&& wt)
		: m_size(wt.m_size)
		, m_bl(std::move(wt.m_bl))
		, m_bf(std::move(wt.m_bf))
		, m_wt(std::move(wt.m_wt))
		, m_bl_rank(std::move(wt.m_bl_rank))
		, m_bf_rank(std::move(wt.m_bf_rank))
		, m_bl_select(std::move(wt.m_bl_select))
		, m_bf_select(std::move(wt.m_bf_select))
		, m_C(std::move(wt.m_C))
		, m_C_bf_rank(std::move(wt.m_C_bf_rank))
	{
		m_bl_rank.set_vector(&m_bl);
		m_bf_rank.set_vector(&m_bf);
		m_bl_select.set_vector(&m_bl);
		m_bf_select.set_vector(&m_bf);
	}

	//! Assignment operator
	wt_rlmn& operator=(const wt_rlmn& wt)
	{
		if (this != &wt) {
			wt_rlmn tmp(wt);
			*this = std::move(tmp);
		}
		return *this;
	}

	//! Assignment move operator
	wt_rlmn& operator=(wt_rlmn&& wt)
	{
		if (this != &wt) {
			m_size	= std::move(wt.m_size);
			m_bl	  = std::move(wt.m_bl);
			m_bf	  = std::move(wt.m_bf);
			m_wt	  = std::move(wt.m_wt);
			m_bl_rank = std::move(wt.m_bl_rank);
			m_bl_rank.set_vector(&m_bl);
			m_bf_rank = std::move(wt.m_bf_rank);
			m_bf_rank.set_vector(&m_bf);
			m_bl_select = std::move(wt.m_bl_select);
			m_bl_select.set_vector(&m_bl);
			m_bf_select = std::move(wt.m_bf_select);
			m_bf_select.set_vector(&m_bf);
			m_C			= std::move(wt.m_C);
			m_C_bf_rank = std::move(wt.m_C_bf_rank);
		}
		return *this;
	}

	//! Returns the size of the original vector.
	size_type size() const { return m_size; }

	//! Returns whether the wavelet tree contains no data.
	bool empty() const { return 0 == m_size; }

	//! Recovers the i-th symbol of the original vector.
	/*! \param i Index in the original vector. \f$i \in [0..size()-1]\f$.
         *  \return The i-th symbol of the original vector.
         *  \par Time complexity
         *        \f$ \Order{H_0} \f$ on average, where \f$ H_0 \f$ is the
         *        zero order entropy of the sequence
         */
	value_type operator[](size_type i) const
	{
		assert(i < size());
		return m_wt[m_bl_rank(i + 1) - 1];
	};

	//! Calculates how many symbols c are in the prefix [0..i-1].
	/*!
         *  \param i Exclusive right bound of the range (\f$i\in[0..size()]\f$).
         *  \param c Symbol c.
         *  \return Number of occurrences of symbol c in the prefix [0..i-1].
         *  \par Time complexity
         *        \f$ \Order{H_0} \f$ on average, where \f$ H_0 \f$ is the
         *        zero order entropy of the sequence
         */
	size_type rank(size_type i, value_type c) const
	{
		assert(i <= size());
		if (i == 0) return 0;
		size_type wt_ex_pos = m_bl_rank(i);
		size_type c_runs	= m_wt.rank(wt_ex_pos, c);
		if (c_runs == 0) return 0;
		if (m_wt[wt_ex_pos - 1] == c) {
			size_type c_run_begin = m_bl_select(wt_ex_pos);
			return m_bf_select(m_C_bf_rank[c] + c_runs) - m_C[c] + i - c_run_begin;
		} else {
			return m_bf_select(m_C_bf_rank[c] + c_runs + 1) - m_C[c];
		}
	};

	//! Calculates how many times symbol wt[i] occurs in the prefix [0..i-1].
	/*!
         *  \param i The index of the symbol.
         *  \return  Pair (rank(wt[i],i),wt[i])
         *    \par Time complexity
         *        \f$ \Order{H_0} \f$
         */
	std::pair<size_type, value_type> inverse_select(size_type i) const
	{
		assert(i < size());
		if (i == 0) {
			return std::make_pair(0, m_wt[0]);
		}
		size_type  wt_ex_pos = m_bl_rank(i + 1);
		auto	   rc		 = m_wt.inverse_select(wt_ex_pos - 1);
		size_type  c_runs	= rc.first + 1;
		value_type c		 = rc.second;
		if (c_runs == 0) return std::make_pair(0, c);
		if (m_wt[wt_ex_pos - 1] == c) {
			size_type c_run_begin = m_bl_select(wt_ex_pos);
			return std::make_pair(m_bf_select(m_C_bf_rank[c] + c_runs) - m_C[c] + i - c_run_begin,
								  c);
		} else {
			return std::make_pair(m_bf_select(m_C_bf_rank[c] + c_runs + 1) - m_C[c], c);
		}
	}

	//! Calculates the ith occurrence of the symbol c in the supported vector.
	/*!
         *  \param i The ith occurrence. \f$i\in [1..rank(size(),c)]\f$.
         *  \param c The symbol c.
         *  \par Time complexity
         *       \f$ \Order{H_0} \f$ on average, where \f$ H_0 \f$ is the zero order
         *        entropy of the sequence
         */
	size_type select(size_type i, value_type c) const
	{
		assert(i > 0);
		assert(i <= rank(size(), c));
		size_type c_runs = m_bf_rank(m_C[c] + i) - m_C_bf_rank[c];
		size_type offset = m_C[c] + i - 1 - m_bf_select(c_runs + m_C_bf_rank[c]);
		return m_bl_select(m_wt.select(c_runs, c) + 1) + offset;
	};

	//! Returns a const_iterator to the first element.
	const_iterator begin() const { return const_iterator(this, 0); }

	//! Returns a const_iterator to the element after the last element.
	const_iterator end() const { return const_iterator(this, size()); }

	//! Serializes the data structure into the given ostream
	size_type
	serialize(std::ostream& out, structure_tree_node* v = nullptr, std::string name = "") const
	{
		structure_tree_node* child = structure_tree::add_child(v, name, util::class_name(*this));
		size_type			 written_bytes = 0;
		written_bytes += write_member(m_size, out, child, "size");
		written_bytes += m_bl.serialize(out, child, "bl");
		written_bytes += m_bf.serialize(out, child, "bf");
		written_bytes += m_wt.serialize(out, child, "wt");
		written_bytes += m_bl_rank.serialize(out, child, "bl_rank");
		written_bytes += m_bf_rank.serialize(out, child, "bf_rank");
		written_bytes += m_bl_select.serialize(out, child, "bl_select");
		written_bytes += m_bf_select.serialize(out, child, "bf_select");
		written_bytes += m_C.serialize(out, child, "C");
		written_bytes += m_C_bf_rank.serialize(out, child, "C_bf_rank");
		structure_tree::add_size(child, written_bytes);
		return written_bytes;
	}

	//! Loads the data structure from the given istream.
	void load(std::istream& in)
	{
		read_member(m_size, in);
		m_bl.load(in);
		m_bf.load(in);
		m_wt.load(in);
		m_bl_rank.load(in, &m_bl);
		m_bf_rank.load(in, &m_bf);
		m_bl_select.load(in, &m_bl);
		m_bf_select.load(in, &m_bf);
		m_C.load(in);
		m_C_bf_rank.load(in);
	}

	//!\brief Serialise (save) via cereal
	template <typename archive_t>
	void CEREAL_SAVE_FUNCTION_NAME(archive_t & ar) const
	{
		ar(CEREAL_NVP(m_size));
		ar(CEREAL_NVP(m_bl));
		ar(CEREAL_NVP(m_bf));
		ar(CEREAL_NVP(m_wt));
		ar(CEREAL_NVP(m_bl_rank));
		ar(CEREAL_NVP(m_bf_rank));
		ar(CEREAL_NVP(m_bl_select));
		ar(CEREAL_NVP(m_bf_select));
		ar(CEREAL_NVP(m_C));
		ar(CEREAL_NVP(m_C_bf_rank));
	}

	//!\brief Load via cereal
	template <typename archive_t>
	void CEREAL_LOAD_FUNCTION_NAME(archive_t & ar)
	{
		ar(CEREAL_NVP(m_size));
		ar(CEREAL_NVP(m_bl));
		ar(CEREAL_NVP(m_bf));
		ar(CEREAL_NVP(m_wt));
		ar(CEREAL_NVP(m_bl_rank));
		m_bl_rank.set_vector(&m_bl);
		ar(CEREAL_NVP(m_bf_rank));
		m_bf_rank.set_vector(&m_bf);
		ar(CEREAL_NVP(m_bl_select));
		m_bl_select.set_vector(&m_bl);
		ar(CEREAL_NVP(m_bf_select));
		m_bf_select.set_vector(&m_bf);
		ar(CEREAL_NVP(m_C));
		ar(CEREAL_NVP(m_C_bf_rank));
	}

	//! Equality operator.
	bool operator==(wt_rlmn const & other) const noexcept
	{
		return (m_size == other.m_size) && (m_bl == other.m_bl) && (m_bf == other.m_bf) &&
		       (m_wt == other.m_wt) && (m_bl_rank == other.m_bl_rank) &&
		       (m_bf_rank == other.m_bf_rank) && (m_bl_select == other.m_bl_select) &&
		       (m_bf_select == other.m_bf_select) && (m_C == other.m_C) &&
		       (m_C_bf_rank == other.m_C_bf_rank);
	}

	//! Inequality operator.
	bool operator!=(wt_rlmn const & other) const noexcept
	{
		return !(*this == other);
	}
};

} // end namespace sdsl
#endif
