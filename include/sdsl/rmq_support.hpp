// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
/*! \file rmq_support.hpp
    \brief rmq_support.hpp contains different range minimum support data structures.
	\author Simon Gog
*/
#ifndef INCLUDED_SDSL_RMQ_SUPPORT
#define INCLUDED_SDSL_RMQ_SUPPORT

/** \defgroup rmq_group Range Minimum/Maximum Support (RMS) */

template <class RandomAccessContainer, bool Minimum> // for range minimum queries
struct min_max_trait {
	static inline bool strict_compare(const typename RandomAccessContainer::value_type v1,
									  const typename RandomAccessContainer::value_type v2)
	{
		return v1 < v2;
	}
	static inline bool compare(const typename RandomAccessContainer::value_type v1,
							   const typename RandomAccessContainer::value_type v2)
	{
		return v1 <= v2;
	}
};

template <class RandomAccessContainer> // for range maximum queries
struct min_max_trait<RandomAccessContainer, false> {
	static inline bool strict_compare(const typename RandomAccessContainer::value_type v1,
									  const typename RandomAccessContainer::value_type v2)
	{
		return v1 > v2;
	}
	static inline bool compare(const typename RandomAccessContainer::value_type v1,
							   const typename RandomAccessContainer::value_type v2)
	{
		return v1 >= v2;
	}
};

#include "rmq_support_sparse_table.hpp"
#include "rmq_succinct_sct.hpp"
#include "rmq_succinct_sada.hpp"

#endif
