// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
/*! \file bp_support_algorithm.hpp
    \brief bp_support_algorithm.hpp contains algorithms for balanced parentheses sequences.
	\author Simon Gog
*/
#ifndef INCLUDED_SDSL_BP_SUPPORT_ALGORITHM
#define INCLUDED_SDSL_BP_SUPPORT_ALGORITHM

#include "int_vector.hpp" // for bit_vector
#include <stack>		  // for calculate_pioneers_bitmap method
#include <map>			  // for calculate_pioneers_bitmap method
#include "sorted_stack_support.hpp"


namespace sdsl {

// This structure contains lookup tables
template <typename T = void>
struct excess {
	struct impl {
		// Given an excess value x in [-8,8] and a 8-bit
		// word w interpreted as parentheses sequence.
		// near_fwd_pos[(x+8)<<8 | w] contains the minimal position
		// p in [0..7] where the excess value x is reached, or 8
		// if x is not reached in w.
		uint8_t near_fwd_pos[(8 - (-8)) * 256];

		// Given an excess value of x in [-8,8] and a 8-bit
		// word w interpreted as parentheses sequence.
		// near_bwd_pos[(x+8)<<8 | w] contains the maximal position
		// p in [0..7] where the excess value x is reached, or 8
		// if x is not reached in w.
		uint8_t near_bwd_pos[(8 - (-8)) * 256];

		// Given a 8-bit word w. word_sum[w] contains the
		// excess value of w.
		int8_t word_sum[256];

		// Given a 8-bit word w. min[w] contains the
		// minimal excess value in w.
		int8_t min[256];

		// Given a 8-bit word w. min_pos_max[w] contains
		// the maximal position p in w, where min[w] is
		// reached
		int8_t min_pos_max[256];

		// Given an excess value x in [1,8] and a 8-bit
		// word w interpreted as parentheses sequence.
		// min_match_pos_packed[w]:[(x-1)*4,x*4] contains
		// the minimal position, where excess value
		// -x is reached and 9, if there is no such position.
		uint32_t min_match_pos_packed[256];

		// Given an excess value x in [1,8] and a 8-bit
		// word w interpreted as parentheses sequence.
		// max_match_pos_packed[w]:[(x-1)*4,x*4] contains
		// the maximal position, where excess value
		// -x is reached and 9, if there is no such position.
		uint32_t max_match_pos_packed[256];

		// Given a 8-bit word w. x=min_and_info[w] contains
		// the following information.
		// * [0..7] the minimum excess value in w + 8 of an opening parenthesis
		// * [8..11] the maximal position of the minimal excess value
		// * [12..15] the number of ones in the word
		// if w != 0, and 17 for w=0.
		uint16_t min_open_excess_info[256];

		impl()
		{
			for (int32_t x = -8; x < 8; ++x) {
				for (uint16_t w = 0; w < 256; ++w) {
					uint16_t i		= (x + 8) << 8 | w;
					near_fwd_pos[i] = 8;
					int8_t p		= 0;
					int8_t excess   = 0;
					do {
						excess += 1 - 2 * ((w & (1 << p)) == 0);
						if (excess == x) {
							near_fwd_pos[i] = p;
							break;
						}
						++p;
					} while (p < 8);

					near_bwd_pos[i] = 8;
					p				= 7;
					excess			= 0;
					do {
						excess += 1 - 2 * ((w & (1 << p)) > 0);
						if (excess == x) {
							near_bwd_pos[i] = p;
							break;
						}
						--p;
					} while (p > -1);
				}
			}
			int_vector<> packed_mins(1, 0, 32);
			int_vector<> packed_maxs(1, 0, 32);
			for (uint16_t w = 0; w < 256; ++w) {
				int8_t   excess					= 0;
				int8_t   rev_excess				= 0;
				int32_t  min_excess_of_open		= 17;
				int32_t  min_excess_of_open_pos = 0;
				uint32_t ones					= 0;
				min[w]							= 8;
				packed_mins[0]					= 0x99999999U;
				packed_maxs[0]					= 0x99999999U;
				packed_mins.width(4);
				packed_maxs.width(4);
				for (uint16_t p = 0; p < 8; ++p) {
					ones += (w & (1 << p)) != 0;
					excess += 1 - 2 * ((w & (1 << p)) == 0);
					if (excess <= min[w]) {
						min[w]		   = excess;
						min_pos_max[w] = p;
					}
					if (excess < 0 and packed_mins[-excess - 1] == 9) {
						packed_mins[-excess - 1] = p;
					}
					if (w & (1 << p) and excess + 8 <= min_excess_of_open) {
						min_excess_of_open	 = excess + 8;
						min_excess_of_open_pos = p;
					}
					rev_excess += 1 - 2 * ((w & (1 << (7 - p))) > 0);
					if (rev_excess < 0 and packed_maxs[-rev_excess - 1] == 9) {
						packed_maxs[-rev_excess - 1] = 7 - p;
					}
				}
				word_sum[w] = excess;
				packed_mins.width(32);
				min_match_pos_packed[w] = packed_mins[0];
				packed_maxs.width(32);
				max_match_pos_packed[w] = packed_maxs[0];
				min_open_excess_info[w] =
				(min_excess_of_open) | (min_excess_of_open_pos << 8) | (ones << 12);
			}
		}
	};
	static impl data;
};

template <typename T>
typename excess<T>::impl excess<T>::data;

//! Calculate pioneers as defined in the paper of Geary et al. (CPM 2004)
/*! \param bp             The balanced parentheses sequence.
 *  \param block_size     Block size.
 *  \return Bitvector which marks the pioneers in bp.
 *  \par Time complexity
 *       \f$ \Order{n \log n} \f$, where \f$ n=\f$bp.size()
 *  \par Space complexity
 *       \f$ \Order{2n + min(block\_size, \frac{n}{block\_size} )\cdot \log n } \f$
 */
inline bit_vector calculate_pioneers_bitmap(const bit_vector& bp, uint64_t block_size)
{
	bit_vector pioneer_bitmap(bp.size(), 0);

	std::stack<uint64_t> opening_parenthesis;
	uint64_t			 blocks = (bp.size() + block_size - 1) / block_size;
	// calculate positions of findclose and findopen pioneers
	for (uint64_t block_nr = 0; block_nr < blocks; ++block_nr) {
		std::map<uint64_t, uint64_t> block_and_position; // for find_open and find_close
		std::map<uint64_t, uint64_t> matching_position;  // for find_open and find_close
		for (uint64_t i = 0, j = block_nr * block_size; i < block_size and j < bp.size();
			 ++i, ++j) {
			if (bp[j]) { //opening parenthesis
				opening_parenthesis.push(j);
			} else { // closing parenthesis
				uint64_t position = opening_parenthesis.top();
				uint64_t blockpos = position / block_size;
				opening_parenthesis.pop();
				block_and_position[blockpos] = position;
				matching_position[blockpos]  = j; // greatest j is pioneer
			}
		}
		for (std::map<uint64_t, uint64_t>::const_iterator it = block_and_position.begin(),
														  end = block_and_position.end(),
														  mit = matching_position.begin();
			 it != end and it->first != block_nr;
			 ++it, ++mit) {
			// opening and closing pioneers are symmetric
			pioneer_bitmap[it->second]  = 1;
			pioneer_bitmap[mit->second] = 1;
		}
	}
	// assert that the sequence is balanced
	assert(opening_parenthesis.empty());
	return pioneer_bitmap;
}

//! Space-efficient version of calculate_pioneers_bitmap
/*! \param bp           The balanced parentheses sequence.
 *  \param block_size   Block size.
 *  \return Bitvector which marks the pioneers in bp.
 *  \par Time complexity
 *       \f$ \Order{n} \f$, where \f$ n=\f$bp.size()
 *  \par Space complexity
 *       \f$ \Order{2n + n} \f$ bits: \f$n\f$ bits for input, \f$n\f$ bits for
 *       output, and \f$n\f$ bits for a succinct stack.
 *  \pre The parentheses sequence represented by bp has to be balanced.
 */
inline bit_vector calculate_pioneers_bitmap_succinct(const bit_vector& bp, uint64_t block_size)
{
	bit_vector pioneer_bitmap(bp.size(), 0);

	sorted_stack_support opening_parenthesis(bp.size());
	uint64_t			 cur_pioneer_block = 0, last_start = 0, last_j = 0, cur_block = 0,
			 first_index_in_block = 0;
	// calculate positions of findclose and findopen pioneers
	for (uint64_t j = 0, new_block = block_size; j < bp.size(); ++j, --new_block) {
		if (!(new_block)) {
			cur_pioneer_block = j / block_size;
			++cur_block;
			first_index_in_block = j;
			new_block			 = block_size;
		}

		if (bp[j]) { // opening parenthesis
			if (/*j < bp.size() is not necessary as the last parenthesis is always a closing one*/
				new_block > 1 and !bp[j + 1]) {
				++j;
				--new_block;
				continue;
			}
			opening_parenthesis.push(j);
		} else {
			assert(!opening_parenthesis.empty());
			uint64_t start = opening_parenthesis.top();
			opening_parenthesis.pop();
			if (start < first_index_in_block) {
				if ((start / block_size) == cur_pioneer_block) {
					pioneer_bitmap[last_start] = pioneer_bitmap[last_j] =
					0; // override false pioneer
				}
				pioneer_bitmap[start] = pioneer_bitmap[j] = 1;
				cur_pioneer_block						  = start / block_size;
				last_start								  = start;
				last_j									  = j;
			}
		}
	}
	// assert that the sequence is balanced
	assert(opening_parenthesis.empty());
	return pioneer_bitmap;
}

//! find_open/find_close for closing/opening parentheses.
/*! \param bp      The balanced parentheses sequence.
 *  \param matches Reference to the result.
 *  \pre bp represents a balanced parentheses sequence.
 *  \par Time complexity
 *       \f$ \Order{n} \f$, where \f$ n=\f$bp.size()
 *  \par Space complexity
 *       \f$ \Order{n + 2n\log n } \f$
 */
template <class int_vector>
void calculate_matches(const bit_vector& bp, int_vector& matches)
{
	matches = int_vector(bp.size(), 0, bits::hi(bp.size()) + 1);
	std::stack<uint64_t> opening_parenthesis;
	for (uint64_t i = 0; i < bp.size(); ++i) {
		if (bp[i]) { // opening parenthesis
			opening_parenthesis.push(i);
		} else { // closing parenthesis
			assert(!opening_parenthesis.empty());
			uint64_t position = opening_parenthesis.top();
			opening_parenthesis.pop();
			matches[i] = position;
			assert(matches[i] == position);
			matches[position] = i;
			assert(matches[position] == i);
		}
	}
	// assert that the sequence is balanced
	assert(opening_parenthesis.empty());
}

//! Calculates enclose answers for a balanced parentheses sequence.
/*! \param bp A bit_vector representing a balanced parentheses sequence.
 *  \param enclose Reference to the result.
 *  \pre bp represents a balanced parentheses sequence.
 *  \par Time complexity
 *       \f$ \Order{n} \f$, where \f$ n=\f$bp.size()
 *  \par Space complexity
 *       \f$ \Order{n + 2n\log n } \f$
 */
template <class int_vector>
void calculate_enclose(const bit_vector& bp, int_vector& enclose)
{
	enclose = int_vector(bp.size(), 0, bits::hi(bp.size()) + 1);
	std::stack<uint64_t> opening_parenthesis;
	for (uint64_t i = 0; i < bp.size(); ++i) {
		if (bp[i]) { // opening parenthesis
			if (!opening_parenthesis.empty()) {
				uint64_t position = opening_parenthesis.top();
				enclose[i]		  = position;
				assert(enclose[i] == position);
			} else
				enclose[i] = bp.size();
			opening_parenthesis.push(i);
		} else { // closing parenthesis
			uint64_t position = opening_parenthesis.top();
			enclose[i]		  = position; // find open answer if i is a closing parenthesis
			opening_parenthesis.pop();
		}
	}
	// assert that the sequence is balanced
	assert(opening_parenthesis.empty());
}

inline uint64_t near_find_close(const bit_vector& bp, const uint64_t i, const uint64_t block_size)
{
	typedef bit_vector::difference_type difference_type;
	difference_type	excess_v = 1;

	const uint64_t end = ((i + 1) / block_size + 1) * block_size;
	const uint64_t l   = (((i + 1) + 7) / 8) * 8;
	const uint64_t r   = (end / 8) * 8;
	for (uint64_t j = i + 1; j < std::min(end, l); ++j) {
		if (bp[j])
			++excess_v;
		else {
			--excess_v;
			if (excess_v == 0) {
				return j;
			}
		}
	}
	const uint64_t* b = bp.data();
	for (uint64_t j = l; j < r; j += 8) {
		if (excess_v <= 8) {
			assert(excess_v > 0);
			uint32_t x =
			excess<>::data.min_match_pos_packed[((*(b + (j >> 6))) >> (j & 0x3F)) & 0xFF];
			uint8_t p = (x >> ((excess_v - 1) << 2)) & 0xF;
			if (p < 9) {
				return j + p;
			}
		}
		excess_v += excess<>::data.word_sum[((*(b + (j >> 6))) >> (j & 0x3F)) & 0xFF];
	}
	for (uint64_t j = std::max(l, r); j < end; ++j) {
		if (bp[j])
			++excess_v;
		else {
			--excess_v;
			if (excess_v == 0) {
				return j;
			}
		}
	}
	return i;
}


inline uint64_t near_find_closing(const bit_vector& bp, uint64_t i, uint64_t closings, const uint64_t block_size)
{
	typedef bit_vector::difference_type difference_type;
	difference_type	excess_v    = 0;
	difference_type	succ_excess = -closings;

	const uint64_t end = (i / block_size + 1) * block_size;
	const uint64_t l   = (((i) + 7) / 8) * 8;
	const uint64_t r   = (end / 8) * 8;
	for (uint64_t j = i; j < std::min(end, l); ++j) {
		if (bp[j])
			++excess_v;
		else {
			--excess_v;
			if (excess_v == succ_excess) {
				return j;
			}
		}
	}
	const uint64_t* b = bp.data();
	for (uint64_t j = l; j < r; j += 8) {
		if (excess_v - succ_excess <= 8) {
			uint32_t x =
			excess<>::data.min_match_pos_packed[((*(b + (j >> 6))) >> (j & 0x3F)) & 0xFF];
			uint8_t p = (x >> (((excess_v - succ_excess) - 1) << 2)) & 0xF;
			if (p < 9) {
				return j + p;
			}
		}
		excess_v += excess<>::data.word_sum[((*(b + (j >> 6))) >> (j & 0x3F)) & 0xFF];
	}
	for (uint64_t j = std::max(l, r); j < end; ++j) {
		if (bp[j])
			++excess_v;
		else {
			--excess_v;
			if (excess_v == succ_excess) {
				return j;
			}
		}
	}
	return i - 1;
}

inline uint64_t near_fwd_excess(const bit_vector&			 bp,
						 uint64_t					 i,
						 bit_vector::difference_type rel,
						 const uint64_t				 block_size)
{
	typedef bit_vector::difference_type difference_type;
	difference_type	excess_v = rel;

	const uint64_t end = (i / block_size + 1) * block_size;
	const uint64_t l   = (((i) + 7) / 8) * 8;
	const uint64_t r   = (end / 8) * 8;
	for (uint64_t j = i; j < std::min(end, l); ++j) {
		excess_v += 1 - 2 * bp[j];
		if (!excess_v) {
			return j;
		}
	}
	excess_v += 8;
	const uint64_t* b = bp.data();
	for (uint64_t j = l; j < r; j += 8) {
		if (excess_v >= 0 and excess_v <= 16) {
			uint32_t x =
			excess<>::data.near_fwd_pos[(excess_v << 8) + (((*(b + (j >> 6))) >> (j & 0x3F)) & 0xFF)];
			if (x < 8) {
				return j + x;
			}
		}
		excess_v -= excess<>::data.word_sum[((*(b + (j >> 6))) >> (j & 0x3F)) & 0xFF];
	}
	excess_v -= 8;
	for (uint64_t j = std::max(l, r); j < end; ++j) {
		excess_v += 1 - 2 * bp[j];
		if (!excess_v) {
			return j;
		}
	}
	return i - 1;
}

//! Calculate the position with minimal excess value in the interval [l..r].
/*! \param bp The bit_vector which represents the parentheses sequence
 *  \param l  The left border of the interval.
 *	\param r  The right border of the interval.
 *  \param min_rel_ex Reference to the relative minimal excess value with regards to excess(bp[l])
 */
inline uint64_t
near_rmq(const bit_vector& bp, uint64_t l, uint64_t r, bit_vector::difference_type& min_rel_ex)
{
	typedef bit_vector::difference_type difference_type;
	const uint64_t						l8	 = (((l + 1) + 7) / 8) * 8;
	const uint64_t						r8	 = (r / 8) * 8;
	difference_type						excess_v = 0;
	difference_type						min_pos  = l;
	min_rel_ex							 = 0;
	for (uint64_t j = l + 1; j < std::min(l8, r + 1); ++j) {
		if (bp[j])
			++excess_v;
		else {
			--excess_v;
			if (excess_v <= min_rel_ex) {
				min_rel_ex = excess_v;
				min_pos	= j;
			}
		}
	}

	const uint64_t* b = bp.data();
	for (uint64_t j = l8; j < r8; j += 8) {
		int8_t x = excess<>::data.min[(((*(b + (j >> 6))) >> (j & 0x3F)) & 0xFF)];
		if ((excess_v + x) <= min_rel_ex) {
			min_rel_ex = excess_v + x;
			min_pos	= j + excess<>::data.min_pos_max[(((*(b + (j >> 6))) >> (j & 0x3F)) & 0xFF)];
		}
		excess_v += excess<>::data.word_sum[((*(b + (j >> 6))) >> (j & 0x3F)) & 0xFF];
	}
	for (uint64_t j = std::max(l8, r8); j < r + 1; ++j) {
		if (bp[j])
			++excess_v;
		else {
			--excess_v;
			if (excess_v <= min_rel_ex) {
				min_rel_ex = excess_v;
				min_pos	= j;
			}
		}
	}
	return min_pos;
}

//! Near backward excess
/* This method searches the maximal parenthesis j, with \f$ j\leq i \f$,
 * such that \f$ excess(j) = excess(i+1)+rel \f$ and i < bp.size()-1
 */
inline uint64_t near_bwd_excess(const bit_vector&			 bp,
						 uint64_t					 i,
						 bit_vector::difference_type rel,
						 const uint64_t				 block_size)
{
	typedef bit_vector::difference_type difference_type;
	difference_type					excess_v = rel;
	const difference_type				begin    = ((difference_type)(i) / block_size) * block_size;
	const difference_type				r	 = ((difference_type)(i) / 8) * 8;
	const difference_type				l	 = ((difference_type)((begin + 7) / 8)) * 8;
	for (difference_type j = i + 1; j >= /*begin*/ std::max(r, begin); --j) {
		if (bp[j])
			++excess_v;
		else
			--excess_v;
		if (!excess_v) return j - 1;
	}

	excess_v += 8;
	const uint64_t* b = bp.data();
	for (difference_type j = r - 8; j >= l; j -= 8) {
		if (excess_v >= 0 and excess_v <= 16) {
			uint32_t x =
			excess<>::data.near_bwd_pos[(excess_v << 8) + (((*(b + (j >> 6))) >> (j & 0x3F)) & 0xFF)];
			if (x < 8) {
				return j + x - 1;
			}
		}
		excess_v += excess<>::data.word_sum[((*(b + (j >> 6))) >> (j & 0x3F)) & 0xFF];
	}
	excess_v -= 8;
	for (difference_type j = std::min(l, r); j > begin; --j) {
		if (bp[j])
			++excess_v;
		else
			--excess_v;
		if (!excess_v) return j - 1;
	}
	if (0 == begin and -1 == rel) {
		return -1;
	}
	return i + 1;
}

inline uint64_t near_find_open(const bit_vector& bp, uint64_t i, const uint64_t block_size)
{
	typedef bit_vector::difference_type difference_type;
	difference_type	      excess_v = -1;
	const difference_type begin    = ((difference_type)(i - 1) / block_size) * block_size;
	const difference_type r	       = ((difference_type)(i - 1) / 8) * 8;
	const difference_type l	       = ((difference_type)((begin + 7) / 8)) * 8;
	for (difference_type j = i - 1; j >= std::max(r, begin); --j) {
		if (bp[j]) {
			if (++excess_v == 0) {
				return j;
			}
		} else
			--excess_v;
	}
	const uint64_t* b = bp.data();
	for (difference_type j = r - 8; j >= l; j -= 8) {
		if (excess_v >= -8) {
			assert(excess_v < 0);
			uint32_t x =
			excess<>::data.max_match_pos_packed[((*(b + (j >> 6))) >> (j & 0x3F)) & 0xFF];
			uint8_t p = (x >> ((-excess_v - 1) << 2)) & 0xF;
			if (p < 9) {
				return j + p;
			}
		}
		excess_v += excess<>::data.word_sum[((*(b + (j >> 6))) >> (j & 0x3F)) & 0xFF];
	}
	for (difference_type j = std::min(l, r) - 1; j >= begin; --j) {
		if (bp[j]) {
			if (++excess_v == 0) {
				return j;
			}
		} else
			--excess_v;
	}
	return i;
}

inline uint64_t near_find_opening(const bit_vector& bp,
						   uint64_t			 i,
						   const uint64_t	openings,
						   const uint64_t	block_size)
{
	typedef bit_vector::difference_type difference_type;
	difference_type						excess_v	= 0;
	difference_type						succ_excess = openings;

	const difference_type begin = ((difference_type)(i) / block_size) * block_size;
	const difference_type r		= ((difference_type)(i) / 8) * 8;
	const difference_type l		= ((difference_type)((begin + 7) / 8)) * 8;
	for (difference_type j = i; j >= std::max(r, begin); --j) {
		if (bp[j]) {
			if (++excess_v == succ_excess) {
				return j;
			}
		} else
			--excess_v;
	}
	const uint64_t* b = bp.data();
	for (difference_type j = r - 8; j >= l; j -= 8) {
		if (succ_excess - excess_v <= 8) {
			assert(succ_excess - excess_v > 0);
			uint32_t x =
			excess<>::data.max_match_pos_packed[((*(b + (j >> 6))) >> (j & 0x3F)) & 0xFF];
			uint8_t p = (x >> ((succ_excess - excess_v - 1) << 2)) & 0xF;
			if (p < 9) {
				return j + p;
			}
		}
		excess_v += excess<>::data.word_sum[((*(b + (j >> 6))) >> (j & 0x3F)) & 0xFF];
	}
	for (difference_type j = std::min(l, r) - 1; j >= begin; --j) {
		if (bp[j]) {
			if (++excess_v == succ_excess) {
				return j;
			}
		} else
			--excess_v;
	}
	return i + 1;
}

//! Find the opening parenthesis of the enclosing pair if this parenthesis is near.
/*!
 * \param bp bit_vector containing the representation of the balanced parentheses sequence.
 * \param i Position of the opening parenthesis for which we search the position of the opening parenthesis of the enclosing parentheses pair.
 * \param block_size Number of entries to search for the corresponding opening parenthesis of the enclosing parentheses pair.
 * \return If no near enclose exists return i, otherwise the position of the opening parenthesis of the enclosing pair.
 * \pre We assert that \f$ bp[i]=1 \f$
 */
// TODO: implement a fast version using lookup-tables of size 8
inline uint64_t near_enclose(const bit_vector& bp, uint64_t i, const uint64_t block_size)
{
	uint64_t opening_parentheses = 1;
	for (uint64_t j = i; j + block_size - 1 > i and j > 0; --j) {
		if (bp[j - 1]) {
			++opening_parentheses;
			if (opening_parentheses == 2) {
				return j - 1;
			}
		} else
			--opening_parentheses;
	}
	return i;
}

inline uint64_t near_rmq_open(const bit_vector& bp, const uint64_t begin, const uint64_t end)
{
	typedef bit_vector::difference_type difference_type;
	difference_type						min_excess = end - begin + 1, ex = 0;
	uint64_t							result = end;

	const uint64_t l = ((begin + 7) / 8) * 8;
	const uint64_t r = (end / 8) * 8;

	for (uint64_t k = begin; k < std::min(end, l); ++k) {
		if (bp[k]) {
			++ex;
			if (ex <= min_excess) {
				result	 = k;
				min_excess = ex;
			}
		} else {
			--ex;
		}
	}
	const uint64_t* b = bp.data();
	for (uint64_t k = l; k < r; k += 8) {
		uint16_t x	= excess<>::data.min_open_excess_info[((*(b + (k >> 6))) >> (k & 0x3F)) & 0xFF];
		int8_t   ones = (x >> 12);
		if (ones) {
			int8_t min_ex = (x & 0xFF) - 8;
			if (ex + min_ex <= min_excess) {
				result	 = k + ((x >> 8) & 0xF);
				min_excess = ex + min_ex;
			}
		}
		ex += ((ones << 1) - 8);
	}
	for (uint64_t k = std::max(r, l); k < end; ++k) {
		if (bp[k]) {
			++ex;
			if (ex <= min_excess) {
				result	 = k;
				min_excess = ex;
			}
		} else {
			--ex;
		}
	}
	if (min_excess <= ex) return result;
	return end;
}


} // end namespace sdsl

#endif
