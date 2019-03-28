// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
/*! \file construct_sa.hpp
    \brief construct_sa.hpp contains an interface to access suffix array construction algorithms
    \author Simon Gog
*/

#ifndef INCLUDED_SDSL_CONSTRUCT_SA
#define INCLUDED_SDSL_CONSTRUCT_SA

#include "config.hpp"
#include "int_vector.hpp"

#include "divsufsort.hpp"

#include "qsufsort.hpp"

#include "construct_sa_se.hpp"
#include "construct_config.hpp"

namespace sdsl {

//! Constructs the Suffix Array (SA) from text over byte-alphabet.
/*! The algorithm constructs the SA and stores it to disk.
 *  \param config Reference to cache configuration
 *  \par Space complexity
 *       Usually less than \f$1.5n \f$ bytes of main memory and
 *       \f$10n \f$ bytes of secondary memory
 *  \pre Text exist in the cache. Keys:
 *         * conf::KEY_TEXT
 *  \post SA exist in the cache. Key
 *         * conf::KEY_SA
 *
 *  This construction method uses less main memory, since data-structures
 *  are only kept in main memory, when random access to them is needed.
 *  Otherwise they are stored on disk. The disk-usage peak of this algorithm
 *  is about 10 times the input.
 *
 *  \par References
 *      [1] T. Beller, M. Zwerger, S. Gog and E. Ohlebusch:
 *          ,,Space-Efficient Construction of the Burrows-Wheeler Transform'',
 *          Proceedings of SPIRE 2013.
 *
 */
inline void construct_sa_se(cache_config& config)
{
	int_vector<8> text;
	load_from_file(text, cache_file_name(conf::KEY_TEXT, config));

	if (text.size() <= 2) {
		// If text is c$ or $ write suffix array [1, 0] or [0]
		int_vector_buffer<> sa(cache_file_name(conf::KEY_SA, config), std::ios::out, 8, 2);
		if (text.size() == 2) {
			sa.push_back(1);
		}
		sa.push_back(0);
	} else {
		_construct_sa_se<int_vector<8>>(text, cache_file_name(conf::KEY_SA, config), 256, 0);
	}
	register_cache_file(conf::KEY_SA, config);
}


namespace algorithm {

//
// Forward declarations
//----------------------------------------------------------

//! Calculates the Suffix Array for a text.
/*!
 * \param c Text (c-string) to calculate the suffix array. The lex. order is given by the ascii-codes of the characters.
 * \param len Length of the text. *(c+len)=0 and for i<len *(c+len)!=0
 * \param sa Reference to a RandomAccessContainer which will contain the result of the calculation.
 * \pre sa.size() has to be equal to len.
 */

template <typename t_int_vec>
void calculate_sa(const unsigned char* c, typename t_int_vec::size_type len, t_int_vec& sa)
{
	typedef typename t_int_vec::size_type size_type;
	constexpr uint8_t					  t_width = t_int_vec::fixed_int_width;
	if (len <= 1) { // handle special case
		sa.width(1);
		sa.resize(len);
		if (len > 0) sa[0] = 0;
		return;
	}
	bool small_file = (sizeof(len) <= 4 or len < 0x7FFFFFFFULL);
	if (small_file) {
		uint8_t sa_width = sa.width();
		if (32 == t_width or (0 == t_width and 32 >= sa_width)) {
			sa.width(32);
			sa.resize(len);
			divsufsort(c, (int32_t*)sa.data(), (int32_t)len);
			// copy integers back to the right positions
			if (sa_width != 32) {
				for (size_type i = 0, p = 0; i < len; ++i, p += sa_width) {
					sa.set_int(p, sa.get_int(i << 5, 32), sa_width);
				}
				sa.width(sa_width);
				sa.resize(len);
			}
		} else {
			if (sa.width() < bits::hi(len) + 1) {
				throw std::logic_error("width of int_vector is to small for the text!!!");
			}
			int_vector<> sufarray(len, 0, 32);
			divsufsort(c, (int32_t*)sufarray.data(), (int32_t)len);
			sa.resize(len);
			for (size_type i = 0; i < len; ++i) {
				sa[i] = sufarray[i];
			}
		}
	} else {
		uint8_t sa_width = sa.width();
		sa.width(64);
		sa.resize(len);
		divsufsort64(c, (int64_t*)sa.data(), len);
		// copy integers back to the right positions
		if (sa_width != 64) {
			for (size_type i = 0, p = 0; i < len; ++i, p += sa_width) {
				sa.set_int(p, sa.get_int(i << 6, 64), sa_width);
			}
			sa.width(sa_width);
			sa.resize(len);
		}
	}
}


} // end namespace algorithm

//! Constructs the Suffix Array (SA) from text over byte- or integer-alphabet.
/*!    The algorithm constructs the SA and stores it to disk.
 *  \tparam t_width Width of the text. 0==integer alphabet, 8=byte alphabet.
 *  \param config    Reference to cache configuration
 *  \par Space complexity
 *      \f$ 5n \f$ byte for t_width=8 and input < 2GB
 *      \f$ 9n \f$ byte for t_width=8 and input > 2GB
 *      \f$ n \log \sigma \f$ bits for t_width=0
 *  \pre Text exist in the cache. Keys:
 *         * conf::KEY_TEXT for t_width=8 or conf::KEY_TEXT_INT for t_width=0
 *  \post SA exist in the cache. Key
 *         * conf::KEY_SA
 *  \par Reference
 *    For t_width=8: DivSufSort (http://code.google.com/p/libdivsufsort/)
 *    For t_width=0: qsufsort (http://www.larsson.dogma.net/qsufsort.c)
 */
template <uint8_t t_width>
void construct_sa(cache_config& config)
{
	static_assert(t_width == 0 or t_width == 8,
				  "construct_sa: width must be `0` for integer alphabet and `8` for byte alphabet");
	const char* KEY_TEXT = key_text_trait<t_width>::KEY_TEXT;
	if (t_width == 8) {
		if (construct_config().byte_algo_sa == LIBDIVSUFSORT) {
			read_only_mapper<t_width> text(KEY_TEXT, config);
			auto					  sa = write_out_mapper<0>::create(
			cache_file_name(conf::KEY_SA, config), 0, bits::hi(text.size()) + 1);
			// call divsufsort
			algorithm::calculate_sa((const unsigned char*)text.data(), text.size(), sa);
		} else if (construct_config().byte_algo_sa == SE_SAIS) {
			construct_sa_se(config);
		}
	} else if (t_width == 0) {
		// call qsufsort
		int_vector<> sa;
		sdsl::qsufsort::construct_sa(sa, cache_file_name(KEY_TEXT, config).c_str(), 0);
		store_to_cache(sa, conf::KEY_SA, config);
	} else {
		std::cerr << "Unknown alphabet type" << std::endl;
	}
}

} // end namespace sdsl

#endif
