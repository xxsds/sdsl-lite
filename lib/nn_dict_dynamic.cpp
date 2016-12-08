// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
#include "sdsl/nn_dict_dynamic.hpp"
#include "sdsl/util.hpp"

namespace sdsl {
namespace util {
void set_zero_bits(nn_dict_dynamic& nn) { util::set_to_value(nn.m_tree, 0); }
} // end util
} // end sdsl
