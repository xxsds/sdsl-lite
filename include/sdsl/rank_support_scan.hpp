// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
/*! \file rank_support_scan.hpp
    \brief rank_support_scan.hpp contains rank_support_scan that support a sdsl::bit_vector with linear time rank information.
    \author Simon Gog
*/
#ifndef INCLUDED_SDSL_RANK_SUPPORT_SCAN
#define INCLUDED_SDSL_RANK_SUPPORT_SCAN

#include "rank_support.hpp"

//! Namespace for the succinct data structure library.
namespace sdsl {

//! A class supporting rank queries in linear time.
/*! \par Space complexity
 *       Constant.
 *  \par Time complexity
 *       Linear in the size of the supported vector.
 *
 *  \tparam t_b       Bit pattern which should be supported. Either `0`,`1`,`10`,`01`.
 *  \tparam t_pat_len Length of the bit pattern.
 * @ingroup rank_support_group
 */
template <uint8_t t_b = 1, uint8_t t_pat_len = 1>
class rank_support_scan : public rank_support {
private:
	static_assert(t_b == 1u or t_b == 0u or t_b == 10u or t_b == 11u,
				  "rank_support_scan: bit pattern must be `0`,`1`,`10` or `01`");
	static_assert(t_pat_len == 1u or t_pat_len == 2u,
				  "rank_support_scan: bit pattern length must be 1 or 2");

public:
	typedef bit_vector bit_vector_type;
	enum { bit_pat = t_b };
	enum { bit_pat_len = t_pat_len };

public:
	explicit rank_support_scan(const bit_vector* v = nullptr) : rank_support(v){};
	rank_support_scan(const rank_support_scan& rs) = default;
	rank_support_scan(rank_support_scan&& rs)	  = default;
	rank_support_scan& operator=(const rank_support_scan& rs) = default;
	rank_support_scan& operator=(rank_support_scan&& rs) = default;
	size_type rank(size_type idx) const;
	size_type operator()(size_type idx) const { return rank(idx); };
	size_type					   size() const { return m_v->size(); };
	size_type
	serialize(std::ostream& out, structure_tree_node* v = nullptr, std::string name = "") const
	{
		return serialize_empty_object(out, v, name, this);
	}
	void load(std::istream&, const int_vector<1>* v = nullptr) { set_vector(v); }
	void set_vector(const bit_vector* v = nullptr) { m_v = v; }
	template <typename archive_t>
	void CEREAL_SAVE_FUNCTION_NAME(archive_t &) const {}
	template <typename archive_t>
	void CEREAL_LOAD_FUNCTION_NAME(archive_t &) {}

	//! Equality operator.
	bool operator==(rank_support_scan const & other) const noexcept
	{
		return (*m_v == *other.m_v);
	}
	
	//! Inequality operator.
	bool operator!=(rank_support_scan const & other) const noexcept
	{
		return !(*this == other);
	}
};

template <uint8_t t_b, uint8_t t_pat_len>
inline typename rank_support_scan<t_b, t_pat_len>::size_type
rank_support_scan<t_b, t_pat_len>::rank(size_type idx) const
{
	assert(m_v != nullptr);
	assert(idx <= m_v->size());
	const uint64_t* p	  = m_v->data();
	size_type		i	  = 0;
	size_type		result = 0;
	while (i + 64 <= idx) {
		result += rank_support_trait<t_b, t_pat_len>::full_word_rank(p, i);
		i += 64;
	}
	return result + rank_support_trait<t_b, t_pat_len>::word_rank(p, idx);
}

} // end namespace sds

#endif // end file
