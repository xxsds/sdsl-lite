// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
/*! \file wavelet_trees.hpp
    \brief wavelet_trees.hpp contains wavelet tree implementations.
	\author Simon Gog
*/
#ifndef INCLUDED_SDSL_WAVELET_TREES
#define INCLUDED_SDSL_WAVELET_TREES

/** \defgroup wt Wavelet Trees (WT)
 *   This group contains data structures for wavelet trees. The following methods are supported:
 *    - []-operator
 *    - rank(i, c)
 *    - select(i, c)
 *    - inverse_select(i)
 */

#include "wt_pc.hpp"
#include "wt_blcd.hpp"
#include "wt_gmr.hpp"
#include "wt_huff.hpp"
#include "wt_hutu.hpp"
#include "wt_int.hpp"
#include "wm_int.hpp"
#include "wt_rlmn.hpp"
#include "wt_ap.hpp"
#include "wt_epr.hpp"
#include "construct.hpp"
#include "wt_algorithm.hpp"

namespace sdsl {

template <class t_bitvector   = bit_vector,
		  class t_rank		  = typename t_bitvector::rank_1_type,
		  class t_select	  = typename t_bitvector::select_1_type,
		  class t_select_zero = typename t_bitvector::select_0_type>
using wt_hutu_int = wt_pc<hutu_shape, t_bitvector, t_rank, t_select, t_select_zero, int_tree<>>;

template <class t_bitvector   = bit_vector,
		  class t_rank		  = typename t_bitvector::rank_1_type,
		  class t_select	  = typename t_bitvector::select_1_type,
		  class t_select_zero = typename t_bitvector::select_0_type>
using wt_huff_int = wt_pc<huff_shape, t_bitvector, t_rank, t_select, t_select_zero, int_tree<>>;

template <class t_bitvector   = bit_vector,
		  class t_rank		  = typename t_bitvector::rank_1_type,
		  class t_select_one  = typename t_bitvector::select_1_type,
		  class t_select_zero = typename t_bitvector::select_0_type>
using wt_blcd_int =
wt_pc<balanced_shape, t_bitvector, t_rank, t_select_one, t_select_zero, int_tree<>>;
}

#endif
