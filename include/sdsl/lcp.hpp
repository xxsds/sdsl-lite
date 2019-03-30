// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
/*! \file lcp.hpp
    \brief lcp.hpp contains classes for lcp information.
    \author Simon Gog
*/
#ifndef INCLUDED_SDSL_LCP
#define INCLUDED_SDSL_LCP

#include "sdsl_concepts.hpp"

#include "int_vector.hpp"
#include "csa_alphabet_strategy.hpp" 
#include "select_support_mcl.hpp"
#include "construct_isa.hpp"
#include <istream>

//! Namespace for the succinct data structure library.
namespace sdsl {

// construct lcp arrays
template <class t_lcp, class t_cst>
void construct_lcp(t_lcp& lcp, const t_cst& cst, cache_config& config)
{
	typename t_lcp::lcp_category tag;
	construct_lcp(lcp, cst, config, tag);
}

template <class t_lcp, class t_cst>
void construct_lcp(t_lcp& lcp, const t_cst&, cache_config& config, lcp_plain_tag)
{
	lcp = t_lcp(config);
}

template <class t_lcp, class t_cst>
void construct_lcp(t_lcp& lcp, const t_cst& cst, cache_config& config, lcp_permuted_tag)
{
	lcp = t_lcp(config, &(cst.csa));
}

template <class t_lcp, class t_cst>
void construct_lcp(t_lcp& lcp, const t_cst& cst, cache_config& config, lcp_tree_compressed_tag)
{
	lcp = t_lcp(config, &cst);
}

template <class t_lcp, class t_cst>
void construct_lcp(t_lcp&		 lcp,
				   const t_cst&  cst,
				   cache_config& config,
				   lcp_tree_and_lf_compressed_tag)
{
	lcp = t_lcp(config, &cst);
}

// copy lcp arrays
template <class t_lcp, class t_cst>
void copy_lcp(t_lcp& lcp, const t_lcp& lcp_c, const t_cst& cst)
{
	typename t_lcp::lcp_category tag;
	copy_lcp(lcp, lcp_c, cst, tag);
}

template <class t_lcp, class t_cst>
void copy_lcp(t_lcp& lcp, const t_lcp& lcp_c, const t_cst&, lcp_plain_tag)
{
	lcp = lcp_c;
}

template <class t_lcp, class t_cst>
void copy_lcp(t_lcp& lcp, const t_lcp& lcp_c, const t_cst& cst, lcp_permuted_tag)
{
	lcp = lcp_c;
	lcp.set_csa(&(cst.csa));
}

template <class t_lcp, class t_cst>
void copy_lcp(t_lcp& lcp, const t_lcp& lcp_c, const t_cst& cst, lcp_tree_compressed_tag)
{
	lcp = lcp_c;
	lcp.set_cst(&cst);
}

template <class t_lcp, class t_cst>
void copy_lcp(t_lcp& lcp, const t_lcp& lcp_c, const t_cst& cst, lcp_tree_and_lf_compressed_tag)
{
	lcp = lcp_c;
	lcp.set_cst(&cst);
}

// move lcp arrays
template <class t_lcp, class t_cst>
void move_lcp(t_lcp&& lcp, t_lcp&& lcp_c, const t_cst& cst)
{
	typename std::remove_reference<t_lcp>::type::lcp_category tag;
	move_lcp(std::forward<t_lcp>(lcp), std::forward<t_lcp>(lcp_c), cst, tag);
}

template <class t_lcp, class t_cst>
void move_lcp(t_lcp&& lcp, t_lcp&& lcp_c, const t_cst&, lcp_plain_tag)
{
	lcp = std::move(lcp_c);
}

template <class t_lcp, class t_cst>
void move_lcp(t_lcp&& lcp, t_lcp&& lcp_c, const t_cst& cst, lcp_permuted_tag)
{
	lcp = std::move(lcp_c);
	lcp.set_csa(&(cst.csa));
}

template <class t_lcp, class t_cst>
void move_lcp(t_lcp&& lcp, t_lcp&& lcp_c, const t_cst& cst, lcp_tree_compressed_tag)
{
	lcp = std::move(lcp_c);
	lcp.set_cst(&cst);
}

template <class t_lcp, class t_cst>
void move_lcp(t_lcp&& lcp, t_lcp&& lcp_c, const t_cst& cst, lcp_tree_and_lf_compressed_tag)
{
	lcp = std::move(lcp_c);
	lcp.set_cst(&cst);
}

// load lcp arrays
template <class t_lcp, class t_cst>
void load_lcp(t_lcp& lcp, std::istream& in, const t_cst& cst)
{
	typename t_lcp::lcp_category tag;
	load_lcp(lcp, in, cst, tag);
}

template <class t_lcp, class t_cst>
void load_lcp(t_lcp& lcp, std::istream& in, const t_cst&, lcp_plain_tag)
{
	lcp.load(in);
}

template <class t_lcp, class t_cst>
void load_lcp(t_lcp& lcp, std::istream& in, const t_cst& cst, lcp_permuted_tag)
{
	lcp.load(in, &(cst.csa));
}

template <class t_lcp, class t_cst>
void load_lcp(t_lcp& lcp, std::istream& in, const t_cst& cst, lcp_tree_compressed_tag)
{
	lcp.load(in, &cst);
}

template <class t_lcp, class t_cst>
void load_lcp(t_lcp& lcp, std::istream& in, const t_cst& cst, lcp_tree_and_lf_compressed_tag)
{
	lcp.load(in, &cst);
}

// set lcp pointers
template <class t_lcp, class t_cst>
void set_lcp_pointer(t_lcp& lcp, const t_cst& cst)
{
	typename t_lcp::lcp_category tag;
	set_lcp_pointer(lcp, cst, tag);
}

template <class t_lcp, class t_cst>
void set_lcp_pointer(t_lcp&, const t_cst&, lcp_plain_tag) {}

template <class t_lcp, class t_cst>
void set_lcp_pointer(t_lcp& lcp, const t_cst& cst, lcp_permuted_tag)
{
	lcp.set_csa(&(cst.csa));
}

template <class t_lcp, class t_cst>
void set_lcp_pointer(t_lcp& lcp, const t_cst& cst, lcp_tree_compressed_tag)
{
	lcp.set_cst(&cst);
}

template <class t_lcp, class t_cst>
void set_lcp_pointer(t_lcp& lcp, const t_cst& cst, lcp_tree_and_lf_compressed_tag)
{
	lcp.set_cst(&cst);
}

} // end namespace sdsl

#include "lcp_support_sada.hpp"  // type (b)
#include "lcp_byte.hpp"			 // type (a)
#include "lcp_wt.hpp"			 // type (a)
#include "lcp_vlc.hpp"			 // type (a)
#include "lcp_dac.hpp"			 // type (a)
#include "lcp_bitcompressed.hpp" // type (a)
#include "lcp_support_tree.hpp"  // type (c)
#include "lcp_support_tree2.hpp" // type (c)


#endif
