// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
#ifndef SDSL_CONFIG
#define SDSL_CONFIG

#include "uintx_t.hpp"
#include <map>
#include <string>

#ifndef MSVC_COMPILER
#define SDSL_UNUSED __attribute__((unused))
#else
#define SDSL_UNUSED
#endif

namespace sdsl {

// forward declarations
namespace util {
    template <typename T>
    std::string to_string(const T& t, int w = 1);
    uint64_t pid();
    uint64_t id();
}

namespace conf // namespace for library constant
{
// size of the buffer for reading and writing data in elements (not in bytes)
const uint64_t SDSL_BLOCK_SIZE = (uint64_t)1 << 22;

constexpr char KEY_BWT[]		 = "bwt";
constexpr char KEY_BWT_INT[]	 = "bwt_int";
constexpr char KEY_SA[]			 = "sa";
constexpr char KEY_CSA[]		 = "csa";
constexpr char KEY_CST[]		 = "cst";
constexpr char KEY_ISA[]		 = "isa";
constexpr char KEY_TEXT[]		 = "text";
constexpr char KEY_TEXT_INT[]	= "text_int";
constexpr char KEY_PSI[]		 = "psi";
constexpr char KEY_LCP[]		 = "lcp";
constexpr char KEY_SAMPLE_CHAR[] = "sample_char";
}

typedef uint64_t int_vector_size_type;

typedef std::map<std::string, std::string> tMSS;

enum format_type { JSON_FORMAT, R_FORMAT, HTML_FORMAT };

enum byte_sa_algo_type { LIBDIVSUFSORT, SE_SAIS };

//! Helper class for construction process
struct cache_config {
	bool delete_files; // Flag which indicates if all files which were created
	bool delete_data;  // Flag which indicates if the original data can be deleted
	// during construction should be deleted.
	std::string dir; // Directory for temporary files.
	std::string id;  // Identifier is part of temporary file names. If
	// id is the empty string, then it will be replace
	// a concatenation of PID and a unique ID inside the
	// current process.
	tMSS file_map; // Files stored during the construction process.
	cache_config(bool		 f_delete_files = true,
				 std::string f_dir			= "./",
				 std::string f_id			= "",
				 tMSS		 f_file_map		= tMSS())
	    : delete_files(f_delete_files), delete_data(false), dir(f_dir), id(f_id), file_map(f_file_map)
    {
	    if ("" == id) {
		    id = sdsl::util::to_string(sdsl::util::pid()) + "_" + sdsl::util::to_string(sdsl::util::id());
	}
}
        
};

//! Helper classes to transform width=0 and width=8 to corresponding text key
template <uint8_t width, typename T=void>
struct key_text_trait_impl {
	static const char* KEY_TEXT;
};

template <typename T>
struct key_text_trait_impl<0,T> {
	static const char* KEY_TEXT;
};

template <typename T>
struct key_text_trait_impl<8,T> {
	static const char* KEY_TEXT;
};

//! Helper classes to transform width=0 and width=8 to corresponding bwt key
template <uint8_t width,typename T=void>
struct key_bwt_trait_impl {
	static const char* KEY_BWT;
};

template <typename T>
struct key_bwt_trait_impl<0,T> {
	static const char* KEY_BWT;
};

template <typename T>
struct key_bwt_trait_impl<8,T> {
	static const char* KEY_BWT;
};

template<typename T>
const char* key_text_trait_impl<0,T>::KEY_TEXT = conf::KEY_TEXT_INT;

template<typename T>
const char* key_text_trait_impl<8,T>::KEY_TEXT = conf::KEY_TEXT;

template<typename T>
const char* key_bwt_trait_impl<0,T>::KEY_BWT = conf::KEY_BWT_INT;

template<typename T>
const char* key_bwt_trait_impl<8,T>::KEY_BWT = conf::KEY_BWT;

template<uint8_t width>
using key_text_trait = key_text_trait_impl<width,void>;

template<uint8_t width>
using key_bwt_trait = key_bwt_trait_impl<width,void>;

}

#endif
