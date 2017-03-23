// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
#ifndef INCLUDED_SDSL_CONSTRUCT_CONFIG
#define INCLUDED_SDSL_CONSTRUCT_CONFIG

#include "config.hpp"

namespace sdsl {

struct construct_config_data {
	byte_sa_algo_type byte_algo_sa = LIBDIVSUFSORT;
};

extern inline construct_config_data& construct_config() {
    static construct_config_data data;
    return data;
}

}

#endif
