// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
#include "sdsl/uint256_t.hpp"

//! Namespace for the succinct data structure library
namespace sdsl {
std::ostream& operator<<(std::ostream& os, const uint256_t& x)
{
	uint64_t X[4] = {(uint64_t)(x.m_high >> 64), (uint64_t)x.m_high, x.m_mid, x.m_lo};
	for (int j = 0; j < 4; ++j) {
		for (int i = 0; i < 16; ++i) {
			os << std::hex << ((X[j] >> 60) & 0xFULL) << std::dec;
			X[j] <<= 4;
		}
	}
	return os;
}
} // end namespace
