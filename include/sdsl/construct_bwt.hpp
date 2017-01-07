// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
/*! \file construct_bwt.hpp
    \brief construct_bwt.hpp contains a space and time efficient construction method for the Burrows and Wheeler Transform (BWT).
    \author Simon Gog
*/
#ifndef INCLUDED_SDSL_CONSTRUCT_BWT
#define INCLUDED_SDSL_CONSTRUCT_BWT

#include "int_vector.hpp"
#include "sfstream.hpp"
#include "util.hpp"
#include "config.hpp" // for cache_config

#include <iostream>
#include <stdexcept>
#include <list>

namespace sdsl {

//! Constructs the Burrows and Wheeler Transform (BWT) from text over byte- or integer-alphabet and suffix array.
/*!	The algorithm constructs the BWT and stores it to disk.
 *  \tparam t_width Width of the text. 0==integer alphabet, 8=byte alphabet.
 *  \param config	Reference to cache configuration
 *  \par Space complexity
 *		\f$ n \log \sigma \f$ bits
 *  \pre Text and Suffix array exist in the cache. Keys:
 *         * conf::KEY_TEXT for t_width=8 or conf::KEY_TEXT_INT for t_width=0
 *         * conf::KEY_SA
 *  \post BWT exist in the cache. Key
 *         * conf::KEY_BWT for t_width=8 or conf::KEY_BWT_INT for t_width=0
 */
template <uint8_t t_width>
void construct_bwt(cache_config& config)
{
	static_assert(
	t_width == 0 or t_width == 8,
	"construct_bwt: width must be `0` for integer alphabet and `8` for byte alphabet");

	typedef int_vector<>::size_type size_type;
	const char*						KEY_TEXT = key_text_trait<t_width>::KEY_TEXT;
	const char*						KEY_BWT  = key_bwt_trait<t_width>::KEY_BWT;

	//  (1) Load text from disk
	read_only_mapper<t_width> text(KEY_TEXT, config);
	size_type				  n			= text.size();
	uint8_t					  bwt_width = text.width();
	std::string				  bwt_file  = cache_file_name(KEY_BWT, config);

	auto gen_bwt = [&n](auto& bwt, auto& text, auto& sa) {
		size_type to_add[2] = {(size_type)-1, n - 1};
		for (size_type i = 0; i < n; ++i) {
			bwt[i] = text[sa[i] + to_add[sa[i] == 0]];
		}
	};
	//  (2) Prepare to stream SA from disc and BWT to disc
	if (is_ram_file(bwt_file)) {
		int_vector_mapper<> sa(conf::KEY_SA, config);
		auto				bwt = write_out_mapper<t_width>::create(bwt_file, n, bwt_width);
		gen_bwt(bwt, text, sa);
	} else {
		size_type			buffer_size = 1000000; // buffer_size is a multiple of 8!
		std::string			sa_file		= cache_file_name(conf::KEY_SA, config);
		int_vector_buffer<> sa_buf(sa_file, std::ios::in, buffer_size);
		auto				bwt = write_out_mapper<t_width>::create(bwt_file, n, bwt_width);
		//  (3) Construct BWT sequentially by streaming SA and random access to text
		gen_bwt(bwt, text, sa_buf);
	}
	register_cache_file(KEY_BWT, config);
}

} // end namespace

#endif
