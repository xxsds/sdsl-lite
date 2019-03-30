// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
/* \file lcp_vlc.hpp
    \brief lcp_vlc.hpp contains an implementation of a (compressed) LCP array.
    \author Simon Gog
*/
#ifndef INCLUDED_SDSL_LCP_VLC
#define INCLUDED_SDSL_LCP_VLC

#include "vlc_vector.hpp"
#include "int_vector.hpp"
#include "iterators.hpp"
#include <iostream>
#include <cassert>
#include <vector>

namespace sdsl {

// A class for a compressed LCP array based on variable-length coding.
/*
 * \tparam t_vlc_vec Type of the underlying variable-length coder.
 */
template <class t_vlc_vec = vlc_vector<>>
class lcp_vlc {
public:
	typedef typename t_vlc_vec::value_type		  value_type;
	typedef random_access_const_iterator<lcp_vlc> const_iterator;
	typedef const_iterator						  iterator;
	typedef const value_type					  const_reference;
	typedef const_reference						  reference;
	typedef const_reference*					  pointer;
	typedef const pointer						  const_pointer;
	typedef typename t_vlc_vec::size_type		  size_type;
	typedef typename t_vlc_vec::difference_type   difference_type;
	typedef t_vlc_vec							  vlc_vec_type;

	typedef lcp_plain_tag lcp_category;
	typedef lcp_tag		  index_category;

	enum { fast_access = 0, text_order = 0, sa_order = 1 };

	template <class Cst>
	using type = lcp_vlc;

private:
	vlc_vec_type m_vec;

public:
	//! Default Constructor
	lcp_vlc() = default;

	//! Copy / Move constructor
	lcp_vlc(const lcp_vlc&) = default;
	lcp_vlc(lcp_vlc&&)		= default;
	lcp_vlc& operator=(const lcp_vlc&) = default;
	lcp_vlc& operator=(lcp_vlc&&) = default;

	//! Construct
	lcp_vlc(cache_config& config, std::string other_key = "")
	{
		std::string lcp_key = conf::KEY_LCP;
		if ("" != other_key) {
			lcp_key = other_key;
		}
		int_vector_buffer<> lcp_buf(cache_file_name(lcp_key, config));
		m_vec = vlc_vec_type(lcp_buf);
	}

	//! Number of elements in the instance.
	size_type size() const { return m_vec.size(); }

	//! Returns the largest size that lcp_vlc can ever have.
	static size_type max_size() { return vlc_vec_type::max_size(); }

	//! Returns if the data strucutre is empty.
	bool empty() const { return m_vec.empty(); }

	//! Returns a const_iterator to the first element.
	const_iterator begin() const { return const_iterator(this, 0); }


	//! Returns a const_iterator to the element after the last element.
	const_iterator end() const { return const_iterator(this, size()); }

	//! []-operator
	inline value_type operator[](size_type i) const { return m_vec[i]; }

	//! Serialize to a stream.
	size_type
	serialize(std::ostream& out, structure_tree_node* v = nullptr, std::string name = "") const
	{
		structure_tree_node* child = structure_tree::add_child(v, name, util::class_name(*this));
		size_type			 written_bytes = 0;
		written_bytes += m_vec.serialize(out, child, "vec");
		structure_tree::add_size(child, written_bytes);
		return written_bytes;
	}

	//! Load from a stream.
	void load(std::istream& in) { m_vec.load(in); }

	template <typename archive_t>
	void CEREAL_SAVE_FUNCTION_NAME(archive_t & ar) const
	{
		ar(CEREAL_NVP(m_vec));
	}

	template <typename archive_t>
	void CEREAL_LOAD_FUNCTION_NAME(archive_t & ar)
	{
		ar(CEREAL_NVP(m_vec));
	}

	//! Equality operator.
	bool operator==(lcp_vlc const & other) const noexcept
	{
		return (m_vec == other.m_vec);
	}

	//! Inequality operator.
	bool operator!=(lcp_vlc const & other) const noexcept
	{
		return !(*this == other);
	}
};

} // end namespace sdsl
#endif
