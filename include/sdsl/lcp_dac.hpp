// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
/*! \file lcp_dac.hpp
    \brief lcp_dac.hpp contains an implementation of a (compressed) LCP array.
    \author Simon Gog
*/
#ifndef INCLUDED_SDSL_LCP_DAC
#define INCLUDED_SDSL_LCP_DAC

#include "lcp.hpp"
#include "vectors.hpp"
#include "rank_support_v5.hpp"

namespace sdsl {

//! A class for the compressed version of LCP information of an suffix array
/*! A dac_vector is used to compress represent the values compressed.
 *  The template parameter are forwarded to the dac_vector.
 *  \tparam t_b    Split block size.
 *  \tparam t_rank Rank structure to navigate between the different levels.
 */
template <uint8_t t_b = 4, typename t_rank = rank_support_v5<>>
using lcp_dac = lcp_vlc<dac_vector<t_b, t_rank>>;

} // end namespace sdsl
#endif
