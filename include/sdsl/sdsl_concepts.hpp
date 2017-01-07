// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
/*! \file sdsl_concepts.hpp
    \brief Contains declarations and definitions of data structure concepts.
	\author Simon Gog
*/
#ifndef INCLUDED_SDSL_CONCEPTS
#define INCLUDED_SDSL_CONCEPTS

#include "uintx_t.hpp" // for uint8_t

namespace sdsl {

struct bv_tag {
}; // bitvector tag
struct iv_tag {
}; // int_vector tag

struct csa_tag {
}; // compressed suffix array (CSAs) tag
struct cst_tag {
}; // compressed suffix tree (CST) tag
struct wt_tag {
}; // wavelet tree tag

struct psi_tag {
}; // tag for CSAs based on the psi function
struct lf_tag {
}; // tag for CSAs based on the LF function

struct csa_member_tag {
}; // tag for text, bwt, LF, \Psi members of CSA

struct lcp_tag {
};
struct lcp_plain_tag {
};
struct lcp_permuted_tag {
};
struct lcp_tree_compressed_tag {
};
struct lcp_tree_and_lf_compressed_tag {
};

struct alphabet_tag {
};
struct byte_alphabet_tag {
	static const uint8_t WIDTH = 8;
};
struct int_alphabet_tag {
	static const uint8_t WIDTH = 0;
};

struct sa_sampling_tag {
};
struct isa_sampling_tag {
};


template <class t_T, class t_r = void>
struct enable_if_type {
	typedef t_r type;
};

template <class t_idx, class t_enable = void>
struct index_tag {
	typedef t_enable type;
};

template <class t_idx>
struct index_tag<t_idx, typename enable_if_type<typename t_idx::index_category>::type> {
	using type = typename t_idx::index_category;
};

template <class t_sampling, class t_enable = void>
struct sampling_tag {
	typedef t_enable type;
};

template <class t_sampling>
struct sampling_tag<t_sampling,
					typename enable_if_type<typename t_sampling::sampling_category>::type> {
	using type = typename t_sampling::sampling_category;
};

template <class t_enc_vec, class t_enable = void>
struct is_enc_vec {
    static constexpr bool value = false;
};

template <class t_enc_vec>
struct is_enc_vec<t_enc_vec, typename enable_if_type<typename t_enc_vec::enc_vec_type>::type> {
    static constexpr bool value = true;
};

template <class t_alphabet, class t_enable = void>
struct is_alphabet {
    static constexpr bool value = false;
};

template<class t_alphabet>
struct is_alphabet<t_alphabet, typename enable_if_type<typename t_alphabet::alphabet_category>::type> {
    static constexpr bool value = true;
};

} // end namespace sdsl

#endif
