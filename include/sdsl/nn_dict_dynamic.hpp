// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
/*! \file nn_dict_dynamic.hpp
    \brief nn_dict_dynamic.hpp contains a class for a dynamic bit vector which also supports the prev and next operations
	\author Timo Beller, Simon Gog
*/

#ifndef INCLUDED_NN_DICT_DYNAMIC
#define INCLUDED_NN_DICT_DYNAMIC

#include <sdsl/int_vector.hpp>
#include <sdsl/util.hpp>

namespace sdsl {

// possible TODO: resize(size_type size)

class nn_dict_dynamic;

namespace util {

inline void set_zero_bits(nn_dict_dynamic& nn);

}

//! A class for a dynamic bit vector which also supports the prev and next operations
class nn_dict_dynamic {
public:
	typedef int_vector<64>::size_type size_type;
	class reference; // forward declaration of inner class

	friend class reference;
	friend void util::set_zero_bits(nn_dict_dynamic& nn);

private:
	uint64_t  m_depth; // Depth of the tree (1 level corresonds to 0, 2 levels corresponds to 1,...)
	uint64_t  m_v_begin_leaves; // Virtual begin of leaves
	size_type m_size;
	int_vector<64> m_offset; // Number of nodes to skip on each level
	int_vector<64> m_tree;   // Tree

public:
	const uint64_t& depth;

	size_type size() const { return m_size; }

	//! Constructor
	/*! \param n Number of supported bits
         */
	nn_dict_dynamic(const uint64_t n = 0) : depth(m_depth)
	{
		m_size = n;
		if (n == 0) return;
		uint64_t level;		// level indicator
		uint64_t nodes = 1; // number of nodes (=64 bit integer)
		uint64_t tmp;		// tmp-variable

		/* Calc depth and begin of leaves */
		m_depth			 = bits::hi(n) / 6; // if, n>0 calculate  \f$ \lfloor log_64(n) \rfloor \f$
		m_v_begin_leaves = (1ULL << (m_depth * 6)) / 63;

		/* Calc how many nodes to skip on each level */
		m_offset = int_vector<64>(m_depth + 2, 0);
		level	= m_depth;
		tmp		 = n;
		while (level) {
			tmp = (tmp + 63) / 64; // get real number of nodes, of the next higher level
			//                  <number of nodes in the full tree>  - <real number of nodes>
			m_offset[level + 1] = (1ULL << (6 * level)) - tmp;
			nodes += tmp;
			--level;
		}

		/* Calc how many nodes to skip up to each level*/
		for (level = 1; level <= m_depth; ++level) {
			m_offset[level] += m_offset[level - 1];
		}

		/* Create Tree incl. leaves */
		m_tree = int_vector<64>(nodes);
	}

	//! Copy constructor
	nn_dict_dynamic(const nn_dict_dynamic& nn)
		: m_depth(nn.m_depth)
		, m_v_begin_leaves(nn.m_v_begin_leaves)
		, m_size(nn.m_size)
		, m_offset(nn.m_offset)
		, m_tree(nn.m_tree)
		, depth(m_depth)
	{
	}

	//! move constructor
	nn_dict_dynamic(nn_dict_dynamic&& nn) : depth(m_depth) { *this = std::move(nn); }

	//! Assignment operator
	nn_dict_dynamic& operator=(const nn_dict_dynamic& nn)
	{
		if (this != &nn) {
			nn_dict_dynamic tmp(nn);
			*this = std::move(tmp);
		}
		return *this;
	}

	//! Assignment move operator
	nn_dict_dynamic& operator=(nn_dict_dynamic&& nn)
	{
		if (this != &nn) {
			m_depth			 = std::move(nn.m_depth);
			m_v_begin_leaves = std::move(nn.m_v_begin_leaves);
			m_size			 = std::move(nn.m_size);
			m_offset		 = std::move(nn.m_offset);
			m_tree			 = std::move(nn.m_tree);
			// set nn to default-constructor state
			nn.m_size			= 0;
			nn.m_depth			= 0;
			nn.m_v_begin_leaves = 0;
		}
		return *this;
	}

	//! Access the bit at index idx
	/*! \param idx Index
         *  \par Precondition
         *    \f$ 0 \leq  idx < size() \f$
         */
	bool operator[](const size_type& idx) const
	{
		uint64_t node = m_tree[(m_v_begin_leaves + (idx >> 6)) - m_offset[m_depth]];
		return (node >> (idx & 0x3F)) & 1;
	}

	inline reference operator[](const size_type& idx) { return reference(this, idx); }


	//! Get the leftmost index \f$i\geq idx\f$ where a bit is set.
	/*! \param idx Left border of the search interval. \f$ 0\leq idx < size()\f$
         *
         *  \return If there exists a leftmost index \f$i\geq idx\f$ where a bit is set,
         *          then \f$i\f$ is returned, otherwise size().
         */
	size_type next(const size_type idx) const
	{
		uint64_t v_node_position; // virtual node position
		uint64_t node;			  // current node
		uint64_t dep = m_depth;   // current depth of node
		uint64_t position;		  // position of the 1-bit

		v_node_position = m_v_begin_leaves + (idx >> 6);
		uint8_t off		= idx & 0x3F; // mod 64

		// Go up until a 1-bit is found
		node = m_tree[v_node_position - m_offset[dep]] >> off;
		while (!node or off == 64) {
			// Not in the root
			if (v_node_position) {
				--dep;
				--v_node_position;
				off = (v_node_position & 0x3F) + 1;
				v_node_position >>= 6;
				node = m_tree[v_node_position - m_offset[dep]] >> off;
			} else {
				return size();
			}
		}
		// Calculate the position of the 1-bit
		position = bits::lo(node) + off;

		// Go down to the leaf
		while (v_node_position < m_v_begin_leaves) {
			++dep;
			v_node_position = (v_node_position << 6) + 1 + position;
			node			= m_tree[v_node_position - m_offset[dep]];

			// Calculate the position of the 1-bit
			position = bits::lo(node);
		}
		return ((v_node_position - m_v_begin_leaves) << 6) + position;
	}

	//! Get the rightmost index \f$i \leq idx\f$ where a bit is set.
	/*! \param idx Right border of the search interval. \f$ 0 \leq idx < size()\f$
         *
         *  \return If there exists a rightmost index \f$i \leq idx\f$ where a bit is set,
         *          then \f$i\f$ is returned, otherwise size().
         */
	size_type prev(const size_type idx) const
	{
		uint64_t v_node_position; // virtual node position
		uint64_t node;			  // current node
		uint64_t dep = m_depth;   // current depth of node
		uint64_t position;		  // position of the 1-bit

		v_node_position = m_v_begin_leaves + (idx >> 6);
		uint8_t off		= idx & 0x3F; // mod 64

		// Go up until a 1-bit is found
		node = m_tree[v_node_position - m_offset[dep]] << (63 - off);
		while (!node or off == (uint8_t)-1) {

			// Not in the root
			if (v_node_position) {
				--dep;
				--v_node_position;

				off = ((uint8_t)(v_node_position & 0x3F)) - 1;
				v_node_position >>= 6;

				node = m_tree[v_node_position - m_offset[dep]] << (63 - off);
			} else {
				return size();
			}
		}
		// Calculate the position of the 1-bit
		position = bits::hi(node) - (63 - off);

		// Go down to the leaf
		while (v_node_position < m_v_begin_leaves) {
			++dep;
			v_node_position = (v_node_position << 6) + 1 + position;
			node			= m_tree[v_node_position - m_offset[dep]];

			// Calculate the position of the 1-bit
			position = bits::hi(node); //-(63-off)
		}
		return ((v_node_position - m_v_begin_leaves) << 6) + position;
	}


	//! Load the data structure
	void load(std::istream& in)
	{
		read_member(m_depth, in);
		read_member(m_v_begin_leaves, in);
		read_member(m_size, in);
		m_offset.load(in);
		m_tree.load(in);
	}

	//! Serialize the data structure
	size_type
	serialize(std::ostream& out, structure_tree_node* v = nullptr, std::string name = "") const
	{
		structure_tree_node* child = structure_tree::add_child(v, name, util::class_name(*this));
		size_type			 written_bytes = 0;
		written_bytes += write_member(m_depth, out, child, "depth");
		written_bytes += write_member(m_v_begin_leaves, out, child, "v_begin_leaves");
		written_bytes += write_member(m_size, out, child, "size");
		written_bytes += m_offset.serialize(out, child, "offset");
		written_bytes += m_tree.serialize(out, child, "tree");
		structure_tree::add_size(child, written_bytes);
		return written_bytes;
	}

	//!\brief Serialise (save) via cereal
	template <typename archive_t>
	void CEREAL_SAVE_FUNCTION_NAME(archive_t & ar) const
	{
		ar(CEREAL_NVP(m_depth));
		ar(CEREAL_NVP(m_v_begin_leaves));
		ar(CEREAL_NVP(m_size));
		ar(CEREAL_NVP(m_offset));
		ar(CEREAL_NVP(m_tree));
	}

	//!\brief Load via cereal
	template <typename archive_t>
	void CEREAL_LOAD_FUNCTION_NAME(archive_t & ar)
	{
		ar(CEREAL_NVP(m_depth));
		ar(CEREAL_NVP(m_v_begin_leaves));
		ar(CEREAL_NVP(m_size));
		ar(CEREAL_NVP(m_offset));
		ar(CEREAL_NVP(m_tree));
	}

	//! Equality operator.
	bool operator==(nn_dict_dynamic const & other) const noexcept
	{
		return (m_depth == other.m_depth) && (m_v_begin_leaves == other.m_v_begin_leaves) &&
		       (m_size == other.m_size) && (m_offset == other.m_offset) &&
		       (m_tree == other.m_tree);
	}

	//! Inequality operator.
	bool operator!=(nn_dict_dynamic const & other) const noexcept
	{
		return !(*this == other);
	}

	class reference {
	private:
		nn_dict_dynamic* m_pbv; // pointer to the bit_vector_nearest_neigbour
		size_type		 m_idx; // virtual node position
	public:
		//! Constructor
		reference(nn_dict_dynamic* pbv, nn_dict_dynamic::size_type idx) : m_pbv(pbv), m_idx(idx){};

		//! Assignment operator for the proxy class
		reference& operator=(bool x)
		{
			uint64_t v_node_position;	  // virtual node position
			uint64_t r_node_position;	  // real    node position
			uint64_t dep = m_pbv->m_depth; // current depth of node

			v_node_position = m_pbv->m_v_begin_leaves + (m_idx >> 6);
			uint8_t offset  = m_idx & 0x3F; // pos mod 64
			if (x) {
				while (true) {
					r_node_position = v_node_position - m_pbv->m_offset[dep];
					uint64_t w		= m_pbv->m_tree[r_node_position];
					if ((w >> offset) & 1) { // if the bit was already set
						return *this;
					} else {
						m_pbv->m_tree[r_node_position] |= (1ULL << offset); // set bit
						if (!w and dep) {									// go up in the tree
							--dep;
							--v_node_position;
							offset = v_node_position & 0x3F;
							v_node_position >>= 6;
						} else {
							return *this;
						}
					}
				}
			} else {
				while (true) {
					r_node_position = v_node_position - m_pbv->m_offset[dep];
					uint64_t w		= m_pbv->m_tree[r_node_position];
					if (!((w >> offset) & 1)) { // if the bit is already 0
						return *this;
					} else {
						m_pbv->m_tree[r_node_position] &= (~(1ULL << offset)); // unset bit
						if (!m_pbv->m_tree[r_node_position] and dep) {		   // go up in the tree
							--dep;
							--v_node_position;
							offset = v_node_position & 0x3F;
							v_node_position >>= 6;
						} else {
							return *this;
						}
					}
				}
			}
			return *this;
		}

		reference& operator=(const reference& x) { return *this = bool(x); }

		//! Cast the reference to a bool
		operator bool() const
		{
			uint64_t node =
			m_pbv
			->m_tree[(m_pbv->m_v_begin_leaves + (m_idx >> 6)) - m_pbv->m_offset[m_pbv->m_depth]];
			return (node >> (m_idx & 0x3F)) & 1;
		}

		bool operator==(const reference& x) const { return bool(*this) == bool(x); }

		bool operator<(const reference& x) const { return !bool(*this) and bool(x); }
	};
};

namespace util {
inline void set_zero_bits(nn_dict_dynamic& nn)  { util::set_to_value(nn.m_tree, 0); }
}





} // end of namespace

#endif // end file
