// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
#include "sdsl/config.hpp"
#include "sdsl/util.hpp"

namespace sdsl {

cache_config::cache_config(bool		   f_delete_files,
						   std::string f_dir,
						   std::string f_id,
						   tMSS		   f_file_map)
	: delete_files(f_delete_files), dir(f_dir), id(f_id), file_map(f_file_map)
{
	if ("" == id) {
		id = util::to_string(util::pid()) + "_" + util::to_string(util::id());
	}
}

template <>
const char* key_text_trait<0>::KEY_TEXT = conf::KEY_TEXT_INT;
template <>
const char* key_text_trait<8>::KEY_TEXT = conf::KEY_TEXT;

template <>
const char* key_bwt_trait<0>::KEY_BWT = conf::KEY_BWT_INT;
template <>
const char* key_bwt_trait<8>::KEY_BWT = conf::KEY_BWT;

} // end namespace sdsl
