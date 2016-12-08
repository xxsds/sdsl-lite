// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
/*! \file suffix_trees.hpp
    \brief suffix_trees.hpp contains generic classes for different suffix tree classes.
	\author Simon Gog
*/
#ifndef INCLUDED_SDSL_SUFFIX_TREES
#define INCLUDED_SDSL_SUFFIX_TREES

/** \defgroup cst Compressed Suffix Trees (CST)
 *   This group contains data structures for compressed suffix trees. The following methods are supported:
 *    - root()
 *    - child(v,c)
 *    - select_child(v)
 *    - select_leaf(i)
 *    - parent(v)
 *    - sl(v)
 *    - lca(v,w)
 *    - ..
 */

#include "cst_sct3.hpp"
#include "cst_sada.hpp"
#include "cst_fully.hpp"

#endif
