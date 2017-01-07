// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
/*! \file construct_isa.hpp
    \brief construct_isa.hpp contains a space and time efficient construction method for the inverse suffix array
	\author Simon Gog
*/
#ifndef INCLUDED_SDSL_CONSTRUCT_ISA
#define INCLUDED_SDSL_CONSTRUCT_ISA

#include "int_vector.hpp"
#include "util.hpp"

#include <iostream>
#include <stdexcept>
#include <list>

namespace sdsl {

void construct_isa(cache_config& config);

} // end namespace

#endif
