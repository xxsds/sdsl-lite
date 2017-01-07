// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
#include "sdsl/louds_tree.hpp"

namespace sdsl {
std::ostream& operator<<(std::ostream& os, const louds_node& v)
{
	os << "(" << v.nr << "," << v.pos << ")";
	return os;
}
}
