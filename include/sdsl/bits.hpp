// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
/*! \file bits.hpp
    \brief bits.hpp contains the sdsl::bits class.
	\author Simon Gog
*/
#ifndef INCLUDED_SDSL_BITS
#define INCLUDED_SDSL_BITS

#include <stdint.h> // for uint64_t uint32_t declaration
#include <iostream> // for cerr
#include <cassert>
#ifdef __SSE4_2__
#include <xmmintrin.h>
#endif
#ifdef __BMI2__
#include <x86intrin.h>
#endif

#ifdef WIN32
#include "iso646.h"
#endif

#ifdef __cpp_constexpr
#   if __cpp_constexpr >= 201304
#       define SDSL_CONSTEXPR constexpr
#   else
#       define SDSL_CONSTEXPR
#   endif
#else
#   define SDSL_CONSTEXPR
#endif

//! Namespace for the succinct data structure library.
namespace sdsl {

//! A helper class for bitwise tricks on 64 bit words.
/*!
	bits is a helper class for bitwise tricks and
	techniques. For the basic tricks and techiques we refer to Donald E. Knuth's
	"The Art of Computer Programming", Volume 4A, Chapter 7.1.3 and
	the informative website of Sean E. Anderson about the topic:
	http://www-graphics.stanford.edu/~seander/bithacks.html .

	We have added new functions like: cnt11 and sel11.

	All members of this class are static variables or methods.
	This class cannot be instantiated.

	\author Simon Gog
 */
template <typename T = void>
struct bits_impl {
	bits_impl() = delete;
	//! 64bit mask with all bits set to 1.
	constexpr static uint64_t all_set{-1ULL};

	//! This constant represents a de Bruijn sequence B(k,n) for k=2 and n=6.
	/*! Details for de Bruijn sequences see
       http://en.wikipedia.org/wiki/De_bruijn_sequence
       deBruijn64 is used in combination with the
       array lt_deBruijn_to_idx.
    */
	constexpr static uint64_t deBruijn64{0x0218A392CD3D5DBFULL};

	//! This table maps a 6-bit subsequence S[idx...idx+5] of constant deBruijn64 to idx.
	/*! \sa deBruijn64
    */
	constexpr static uint32_t lt_deBruijn_to_idx[64] = {
	0,  1,  2,  7,  3,  13, 8,  19, 4,  25, 14, 28, 9,  34, 20, 40, 5,  17, 26, 38, 15, 46,
	29, 48, 10, 31, 35, 54, 21, 50, 41, 57, 63, 6,  12, 18, 24, 27, 33, 39, 16, 37, 45, 47,
	30, 53, 49, 56, 62, 11, 23, 32, 36, 44, 52, 55, 61, 22, 43, 51, 60, 42, 59, 58};


	//! Array containing Fibonacci numbers less than \f$2^64\f$.
	constexpr static uint64_t lt_fib[92] = {1,
											2,
											3,
											5,
											8,
											13,
											21,
											34,
											55,
											89,
											144,
											233,
											377,
											610,
											987,
											1597,
											2584,
											4181,
											6765,
											10946,
											17711,
											28657,
											46368,
											75025,
											121393,
											196418,
											317811,
											514229,
											832040,
											1346269,
											2178309,
											3524578,
											5702887,
											9227465,
											14930352,
											24157817,
											39088169,
											63245986,
											102334155,
											165580141,
											267914296,
											433494437,
											701408733,
											1134903170,
											1836311903,
											2971215073ULL,
											0x11e8d0a40ULL,
											0x1cfa62f21ULL,
											0x2ee333961ULL,
											0x4bdd96882ULL,
											0x7ac0ca1e3ULL,
											0xc69e60a65ULL,
											0x1415f2ac48ULL,
											0x207fd8b6adULL,
											0x3495cb62f5ULL,
											0x5515a419a2ULL,
											0x89ab6f7c97ULL,
											0xdec1139639ULL,
											0x1686c8312d0ULL,
											0x2472d96a909ULL,
											0x3af9a19bbd9ULL,
											0x5f6c7b064e2ULL,
											0x9a661ca20bbULL,
											0xf9d297a859dULL,
											0x19438b44a658ULL,
											0x28e0b4bf2bf5ULL,
											0x42244003d24dULL,
											0x6b04f4c2fe42ULL,
											0xad2934c6d08fULL,
											0x1182e2989ced1ULL,
											0x1c5575e509f60ULL,
											0x2dd8587da6e31ULL,
											0x4a2dce62b0d91ULL,
											0x780626e057bc2ULL,
											0xc233f54308953ULL,
											0x13a3a1c2360515ULL,
											0x1fc6e116668e68ULL,
											0x336a82d89c937dULL,
											0x533163ef0321e5ULL,
											0x869be6c79fb562ULL,
											0xd9cd4ab6a2d747ULL,
											0x16069317e428ca9ULL,
											0x23a367c34e563f0ULL,
											0x39a9fadb327f099ULL,
											0x5d4d629e80d5489ULL,
											0x96f75d79b354522ULL,
											0xf444c01834299abULL,
											0x18b3c1d91e77decdULL,
											0x27f80ddaa1ba7878ULL,
											0x40abcfb3c0325745ULL,
											0x68a3dd8e61eccfbdULL,
											0xa94fad42221f2702ULL};


	//! Lookup table for byte popcounts.
	constexpr static uint8_t lt_cnt[256] = {
	0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8};


	//! Lookup table for most significant set bit in a byte.
	constexpr static uint32_t lt_hi[256] = {
	0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7};


	//! lo_set[i] is a 64-bit word with the i least significant bits set and the high bits not set.
	/*! lo_set[0] = 0ULL, lo_set[1]=1ULL, lo_set[2]=3ULL...
     */
	constexpr static uint64_t lo_set[65] = {
	0x0000000000000000ULL, 0x0000000000000001ULL, 0x0000000000000003ULL, 0x0000000000000007ULL,
	0x000000000000000FULL, 0x000000000000001FULL, 0x000000000000003FULL, 0x000000000000007FULL,
	0x00000000000000FFULL, 0x00000000000001FFULL, 0x00000000000003FFULL, 0x00000000000007FFULL,
	0x0000000000000FFFULL, 0x0000000000001FFFULL, 0x0000000000003FFFULL, 0x0000000000007FFFULL,
	0x000000000000FFFFULL, 0x000000000001FFFFULL, 0x000000000003FFFFULL, 0x000000000007FFFFULL,
	0x00000000000FFFFFULL, 0x00000000001FFFFFULL, 0x00000000003FFFFFULL, 0x00000000007FFFFFULL,
	0x0000000000FFFFFFULL, 0x0000000001FFFFFFULL, 0x0000000003FFFFFFULL, 0x0000000007FFFFFFULL,
	0x000000000FFFFFFFULL, 0x000000001FFFFFFFULL, 0x000000003FFFFFFFULL, 0x000000007FFFFFFFULL,
	0x00000000FFFFFFFFULL, 0x00000001FFFFFFFFULL, 0x00000003FFFFFFFFULL, 0x00000007FFFFFFFFULL,
	0x0000000FFFFFFFFFULL, 0x0000001FFFFFFFFFULL, 0x0000003FFFFFFFFFULL, 0x0000007FFFFFFFFFULL,
	0x000000FFFFFFFFFFULL, 0x000001FFFFFFFFFFULL, 0x000003FFFFFFFFFFULL, 0x000007FFFFFFFFFFULL,
	0x00000FFFFFFFFFFFULL, 0x00001FFFFFFFFFFFULL, 0x00003FFFFFFFFFFFULL, 0x00007FFFFFFFFFFFULL,
	0x0000FFFFFFFFFFFFULL, 0x0001FFFFFFFFFFFFULL, 0x0003FFFFFFFFFFFFULL, 0x0007FFFFFFFFFFFFULL,
	0x000FFFFFFFFFFFFFULL, 0x001FFFFFFFFFFFFFULL, 0x003FFFFFFFFFFFFFULL, 0x007FFFFFFFFFFFFFULL,
	0x00FFFFFFFFFFFFFFULL, 0x01FFFFFFFFFFFFFFULL, 0x03FFFFFFFFFFFFFFULL, 0x07FFFFFFFFFFFFFFULL,
	0x0FFFFFFFFFFFFFFFULL, 0x1FFFFFFFFFFFFFFFULL, 0x3FFFFFFFFFFFFFFFULL, 0x7FFFFFFFFFFFFFFFULL,
	0xFFFFFFFFFFFFFFFFULL};


	//! lo_unset[i] is a 64-bit word with the i least significant bits not set and the high bits set.
	/*! lo_unset[0] = FFFFFFFFFFFFFFFFULL, lo_unset_set[1]=FFFFFFFFFFFFFFFEULL, ...
     */
	constexpr static uint64_t lo_unset[65] = {
	0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFEULL, 0xFFFFFFFFFFFFFFFCULL, 0xFFFFFFFFFFFFFFF8ULL,
	0xFFFFFFFFFFFFFFF0ULL, 0xFFFFFFFFFFFFFFE0ULL, 0xFFFFFFFFFFFFFFC0ULL, 0xFFFFFFFFFFFFFF80ULL,
	0xFFFFFFFFFFFFFF00ULL, 0xFFFFFFFFFFFFFE00ULL, 0xFFFFFFFFFFFFFC00ULL, 0xFFFFFFFFFFFFF800ULL,
	0xFFFFFFFFFFFFF000ULL, 0xFFFFFFFFFFFFE000ULL, 0xFFFFFFFFFFFFC000ULL, 0xFFFFFFFFFFFF8000ULL,
	0xFFFFFFFFFFFF0000ULL, 0xFFFFFFFFFFFE0000ULL, 0xFFFFFFFFFFFC0000ULL, 0xFFFFFFFFFFF80000ULL,
	0xFFFFFFFFFFF00000ULL, 0xFFFFFFFFFFE00000ULL, 0xFFFFFFFFFFC00000ULL, 0xFFFFFFFFFF800000ULL,
	0xFFFFFFFFFF000000ULL, 0xFFFFFFFFFE000000ULL, 0xFFFFFFFFFC000000ULL, 0xFFFFFFFFF8000000ULL,
	0xFFFFFFFFF0000000ULL, 0xFFFFFFFFE0000000ULL, 0xFFFFFFFFC0000000ULL, 0xFFFFFFFF80000000ULL,
	0xFFFFFFFF00000000ULL, 0xFFFFFFFE00000000ULL, 0xFFFFFFFC00000000ULL, 0xFFFFFFF800000000ULL,
	0xFFFFFFF000000000ULL, 0xFFFFFFE000000000ULL, 0xFFFFFFC000000000ULL, 0xFFFFFF8000000000ULL,
	0xFFFFFF0000000000ULL, 0xFFFFFE0000000000ULL, 0xFFFFFC0000000000ULL, 0xFFFFF80000000000ULL,
	0xFFFFF00000000000ULL, 0xFFFFE00000000000ULL, 0xFFFFC00000000000ULL, 0xFFFF800000000000ULL,
	0xFFFF000000000000ULL, 0xFFFE000000000000ULL, 0xFFFC000000000000ULL, 0xFFF8000000000000ULL,
	0xFFF0000000000000ULL, 0xFFE0000000000000ULL, 0xFFC0000000000000ULL, 0xFF80000000000000ULL,
	0xFF00000000000000ULL, 0xFE00000000000000ULL, 0xFC00000000000000ULL, 0xF800000000000000ULL,
	0xF000000000000000ULL, 0xE000000000000000ULL, 0xC000000000000000ULL, 0x8000000000000000ULL,
	0x0000000000000000ULL};

	//! Lookup table for least significant set bit in a byte.
	constexpr static uint8_t lt_lo[256] = {
	0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00, 0x03, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00,
	0x04, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00, 0x03, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00,
	0x05, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00, 0x03, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00,
	0x04, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00, 0x03, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00,
	0x06, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00, 0x03, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00,
	0x04, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00, 0x03, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00,
	0x05, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00, 0x03, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00,
	0x04, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00, 0x03, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00,
	0x07, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00, 0x03, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00,
	0x04, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00, 0x03, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00,
	0x05, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00, 0x03, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00,
	0x04, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00, 0x03, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00,
	0x06, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00, 0x03, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00,
	0x04, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00, 0x03, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00,
	0x05, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00, 0x03, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00,
	0x04, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00, 0x03, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00};

	//! Lookup table for select on bytes.
	/*! Entry at idx = 256*j + i equals the position of the
        (j+1)-th set bit in byte i. Positions lie in the range \f$[0..7]\f$.
     */
	constexpr static uint8_t lt_sel[256 * 8] = {
	0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
	5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,

	0, 0, 0, 1, 0, 2, 2, 1, 0, 3, 3, 1, 3, 2, 2, 1, 0, 4, 4, 1, 4, 2, 2, 1, 4, 3, 3, 1, 3, 2, 2, 1,
	0, 5, 5, 1, 5, 2, 2, 1, 5, 3, 3, 1, 3, 2, 2, 1, 5, 4, 4, 1, 4, 2, 2, 1, 4, 3, 3, 1, 3, 2, 2, 1,
	0, 6, 6, 1, 6, 2, 2, 1, 6, 3, 3, 1, 3, 2, 2, 1, 6, 4, 4, 1, 4, 2, 2, 1, 4, 3, 3, 1, 3, 2, 2, 1,
	6, 5, 5, 1, 5, 2, 2, 1, 5, 3, 3, 1, 3, 2, 2, 1, 5, 4, 4, 1, 4, 2, 2, 1, 4, 3, 3, 1, 3, 2, 2, 1,
	0, 7, 7, 1, 7, 2, 2, 1, 7, 3, 3, 1, 3, 2, 2, 1, 7, 4, 4, 1, 4, 2, 2, 1, 4, 3, 3, 1, 3, 2, 2, 1,
	7, 5, 5, 1, 5, 2, 2, 1, 5, 3, 3, 1, 3, 2, 2, 1, 5, 4, 4, 1, 4, 2, 2, 1, 4, 3, 3, 1, 3, 2, 2, 1,
	7, 6, 6, 1, 6, 2, 2, 1, 6, 3, 3, 1, 3, 2, 2, 1, 6, 4, 4, 1, 4, 2, 2, 1, 4, 3, 3, 1, 3, 2, 2, 1,
	6, 5, 5, 1, 5, 2, 2, 1, 5, 3, 3, 1, 3, 2, 2, 1, 5, 4, 4, 1, 4, 2, 2, 1, 4, 3, 3, 1, 3, 2, 2, 1,

	0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 3, 0, 3, 3, 2, 0, 0, 0, 4, 0, 4, 4, 2, 0, 4, 4, 3, 4, 3, 3, 2,
	0, 0, 0, 5, 0, 5, 5, 2, 0, 5, 5, 3, 5, 3, 3, 2, 0, 5, 5, 4, 5, 4, 4, 2, 5, 4, 4, 3, 4, 3, 3, 2,
	0, 0, 0, 6, 0, 6, 6, 2, 0, 6, 6, 3, 6, 3, 3, 2, 0, 6, 6, 4, 6, 4, 4, 2, 6, 4, 4, 3, 4, 3, 3, 2,
	0, 6, 6, 5, 6, 5, 5, 2, 6, 5, 5, 3, 5, 3, 3, 2, 6, 5, 5, 4, 5, 4, 4, 2, 5, 4, 4, 3, 4, 3, 3, 2,
	0, 0, 0, 7, 0, 7, 7, 2, 0, 7, 7, 3, 7, 3, 3, 2, 0, 7, 7, 4, 7, 4, 4, 2, 7, 4, 4, 3, 4, 3, 3, 2,
	0, 7, 7, 5, 7, 5, 5, 2, 7, 5, 5, 3, 5, 3, 3, 2, 7, 5, 5, 4, 5, 4, 4, 2, 5, 4, 4, 3, 4, 3, 3, 2,
	0, 7, 7, 6, 7, 6, 6, 2, 7, 6, 6, 3, 6, 3, 3, 2, 7, 6, 6, 4, 6, 4, 4, 2, 6, 4, 4, 3, 4, 3, 3, 2,
	7, 6, 6, 5, 6, 5, 5, 2, 6, 5, 5, 3, 5, 3, 3, 2, 6, 5, 5, 4, 5, 4, 4, 2, 5, 4, 4, 3, 4, 3, 3, 2,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 4, 0, 4, 4, 3,
	0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 5, 0, 5, 5, 3, 0, 0, 0, 5, 0, 5, 5, 4, 0, 5, 5, 4, 5, 4, 4, 3,
	0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 6, 0, 6, 6, 3, 0, 0, 0, 6, 0, 6, 6, 4, 0, 6, 6, 4, 6, 4, 4, 3,
	0, 0, 0, 6, 0, 6, 6, 5, 0, 6, 6, 5, 6, 5, 5, 3, 0, 6, 6, 5, 6, 5, 5, 4, 6, 5, 5, 4, 5, 4, 4, 3,
	0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 7, 0, 7, 7, 3, 0, 0, 0, 7, 0, 7, 7, 4, 0, 7, 7, 4, 7, 4, 4, 3,
	0, 0, 0, 7, 0, 7, 7, 5, 0, 7, 7, 5, 7, 5, 5, 3, 0, 7, 7, 5, 7, 5, 5, 4, 7, 5, 5, 4, 5, 4, 4, 3,
	0, 0, 0, 7, 0, 7, 7, 6, 0, 7, 7, 6, 7, 6, 6, 3, 0, 7, 7, 6, 7, 6, 6, 4, 7, 6, 6, 4, 6, 4, 4, 3,
	0, 7, 7, 6, 7, 6, 6, 5, 7, 6, 6, 5, 6, 5, 5, 3, 7, 6, 6, 5, 6, 5, 5, 4, 6, 5, 5, 4, 5, 4, 4, 3,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 5, 0, 5, 5, 4,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 6, 0, 6, 6, 4,
	0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 6, 0, 6, 6, 5, 0, 0, 0, 6, 0, 6, 6, 5, 0, 6, 6, 5, 6, 5, 5, 4,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 7, 0, 7, 7, 4,
	0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 7, 0, 7, 7, 5, 0, 0, 0, 7, 0, 7, 7, 5, 0, 7, 7, 5, 7, 5, 5, 4,
	0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 7, 0, 7, 7, 6, 0, 0, 0, 7, 0, 7, 7, 6, 0, 7, 7, 6, 7, 6, 6, 4,
	0, 0, 0, 7, 0, 7, 7, 6, 0, 7, 7, 6, 7, 6, 6, 5, 0, 7, 7, 6, 7, 6, 6, 5, 7, 6, 6, 5, 6, 5, 5, 4,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 6, 0, 6, 6, 5,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 7, 0, 7, 7, 5,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 7, 0, 7, 7, 6,
	0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 7, 0, 7, 7, 6, 0, 0, 0, 7, 0, 7, 7, 6, 0, 7, 7, 6, 7, 6, 6, 5,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 7, 0, 7, 7, 6,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7};

	//! Use to help to decide if a prefix sum stored in a byte overflows.
	constexpr static uint64_t ps_overflow[65] = {
	0x8080808080808080ULL, 0x7f7f7f7f7f7f7f7fULL, 0x7e7e7e7e7e7e7e7eULL, 0x7d7d7d7d7d7d7d7dULL,
	0x7c7c7c7c7c7c7c7cULL, 0x7b7b7b7b7b7b7b7bULL, 0x7a7a7a7a7a7a7a7aULL, 0x7979797979797979ULL,
	0x7878787878787878ULL, 0x7777777777777777ULL, 0x7676767676767676ULL, 0x7575757575757575ULL,
	0x7474747474747474ULL, 0x7373737373737373ULL, 0x7272727272727272ULL, 0x7171717171717171ULL,
	0x7070707070707070ULL, 0x6f6f6f6f6f6f6f6fULL, 0x6e6e6e6e6e6e6e6eULL, 0x6d6d6d6d6d6d6d6dULL,
	0x6c6c6c6c6c6c6c6cULL, 0x6b6b6b6b6b6b6b6bULL, 0x6a6a6a6a6a6a6a6aULL, 0x6969696969696969ULL,
	0x6868686868686868ULL, 0x6767676767676767ULL, 0x6666666666666666ULL, 0x6565656565656565ULL,
	0x6464646464646464ULL, 0x6363636363636363ULL, 0x6262626262626262ULL, 0x6161616161616161ULL,
	0x6060606060606060ULL, 0x5f5f5f5f5f5f5f5fULL, 0x5e5e5e5e5e5e5e5eULL, 0x5d5d5d5d5d5d5d5dULL,
	0x5c5c5c5c5c5c5c5cULL, 0x5b5b5b5b5b5b5b5bULL, 0x5a5a5a5a5a5a5a5aULL, 0x5959595959595959ULL,
	0x5858585858585858ULL, 0x5757575757575757ULL, 0x5656565656565656ULL, 0x5555555555555555ULL,
	0x5454545454545454ULL, 0x5353535353535353ULL, 0x5252525252525252ULL, 0x5151515151515151ULL,
	0x5050505050505050ULL, 0x4f4f4f4f4f4f4f4fULL, 0x4e4e4e4e4e4e4e4eULL, 0x4d4d4d4d4d4d4d4dULL,
	0x4c4c4c4c4c4c4c4cULL, 0x4b4b4b4b4b4b4b4bULL, 0x4a4a4a4a4a4a4a4aULL, 0x4949494949494949ULL,
	0x4848484848484848ULL, 0x4747474747474747ULL, 0x4646464646464646ULL, 0x4545454545454545ULL,
	0x4444444444444444ULL, 0x4343434343434343ULL, 0x4242424242424242ULL, 0x4141414141414141ULL,
	0x4040404040404040ULL};

	//! Counts the number of set bits in x.
	/*! \param  x 64-bit word
        \return Number of set bits.
     */
	SDSL_CONSTEXPR static uint64_t cnt(uint64_t x);

	//! Position of the most significant set bit the 64-bit word x
	/*! \param x 64-bit word
        \return The position (in 0..63) of the most significant set bit
                in `x` or 0 if x equals 0.
    	\sa sel, lo
    */
	SDSL_CONSTEXPR static uint32_t hi(uint64_t x);

	//! Calculates the position of the rightmost 1-bit in the 64bit integer x if it exists
	/*! \param x 64 bit integer.
    	\return The position (in 0..63) of the rightmost 1-bit in the 64bit integer x if
    	        x>0 and 0 if x equals 0.
    	\sa sel, hi
    */
	SDSL_CONSTEXPR static uint32_t lo(uint64_t x);

	//! Counts the number of 1-bits in the 32bit integer x.
	/*! This function is a variant of the method cnt. If
    	32bit multiplication is fast, this method beats the cnt.
    	for 32bit integers.
    	\param x 64bit integer to count the bits.
    	\return The number of 1-bits in x.
     */
	SDSL_CONSTEXPR static uint32_t cnt32(uint32_t x);

	//! Count the number of consecutive and distinct 11 in the 64bit integer x.
	/*!
      	\param x 64bit integer to count the terminating sequence 11 of a Fibonacci code.
    	\param c Carry equals msb of the previous 64bit integer.
     */
	SDSL_CONSTEXPR static uint32_t cnt11(uint64_t x, uint64_t& c);

	//! Count the number of consecutive and distinct 11 in the 64bit integer x.
	/*!
      	\param x 64bit integer to count the terminating sequence 11 of a Fibonacci code.
     */
	SDSL_CONSTEXPR static uint32_t cnt11(uint64_t x);

	//! Count 10 bit pairs in the word x.
	/*!
     * \param x 64bit integer to count the 10 bit pairs.
     * \param c Carry equals msb of the previous 64bit integer.
     */
	SDSL_CONSTEXPR static uint32_t cnt10(uint64_t x, uint64_t& c);

	//! Count 01 bit pairs in the word x.
	/*!
     * \param x 64bit integer to count the 01 bit pairs.
     * \param c Carry equals msb of the previous 64bit integer.
     */
	SDSL_CONSTEXPR static uint32_t cnt01(uint64_t x, uint64_t& c);

	//! Map all 10 bit pairs to 01 or 1 if c=1 and the lsb=0. All other pairs are mapped to 00.
	SDSL_CONSTEXPR static uint64_t map10(uint64_t x, uint64_t c = 0);

	//! Map all 01 bit pairs to 01 or 1 if c=1 and the lsb=0. All other pairs are mapped to 00.
	SDSL_CONSTEXPR static uint64_t map01(uint64_t x, uint64_t c = 1);

	//! Calculate the position of the i-th rightmost 1 bit in the 64bit integer x
	/*!
      	\param x 64bit integer.
    	\param i Argument i must be in the range \f$[1..cnt(x)]\f$.
    	\pre Argument i must be in the range \f$[1..cnt(x)]\f$.
      	\sa hi, lo
     */
	SDSL_CONSTEXPR static uint32_t sel(uint64_t x, uint32_t i);
	SDSL_CONSTEXPR static uint32_t _sel(uint64_t x, uint32_t i);

	//! Calculates the position of the i-th rightmost 11-bit-pattern which terminates a Fibonacci coded integer in x.
	/*!	\param x 64 bit integer.
        \param i Index of 11-bit-pattern. \f$i \in [1..cnt11(x)]\f$
    	\param c Carry bit from word before
     	\return The position (in 1..63) of the i-th 11-bit-pattern which terminates a Fibonacci coded integer in x if
    	        x contains at least i 11-bit-patterns and a undefined value otherwise.
        \sa cnt11, hi11, sel

     */
	SDSL_CONSTEXPR static uint32_t sel11(uint64_t x, uint32_t i, uint32_t c = 0);

	//! Calculates the position of the leftmost 11-bit-pattern which terminates a Fibonacci coded integer in x.
	/*! \param x 64 bit integer.
        \return The position (in 1..63) of the leftmost 1 of the leftmost 11-bit-pattern which
    	        terminates a Fibonacci coded integer in x if x contains a 11-bit-pattern
    			and 0 otherwise.
    	\sa cnt11, sel11
    */
	SDSL_CONSTEXPR static uint32_t hi11(uint64_t x);

	//! Writes value x to an bit position in an array.
	SDSL_CONSTEXPR static void
	write_int(uint64_t* word, uint64_t x, const uint8_t offset = 0, const uint8_t len = 64);

	//! Writes value x to an bit position in an array and moves the bit-pointer.
	SDSL_CONSTEXPR static void write_int_and_move(uint64_t*& word, uint64_t x, uint8_t& offset, const uint8_t len);

	//! Reads a value from a bit position in an array.
	SDSL_CONSTEXPR static uint64_t read_int(const uint64_t* word, uint8_t offset = 0, const uint8_t len = 64);

	//! Reads a value from a bit position in an array and moved the bit-pointer.
	SDSL_CONSTEXPR static uint64_t
	read_int_and_move(const uint64_t*& word, uint8_t& offset, const uint8_t len = 64);

	//! Reads an unary decoded value from a bit position in an array.
	SDSL_CONSTEXPR static uint64_t read_unary(const uint64_t* word, uint8_t offset = 0);

	//! Reads an unary decoded value from a bit position in an array and moves the bit-pointer.
	SDSL_CONSTEXPR static uint64_t read_unary_and_move(const uint64_t*& word, uint8_t& offset);

	//! Move the bit-pointer (=uint64_t word and offset) `len` to the right.
	/*!\param word   64-bit word part of the bit pointer
     * \param offset Offset part of the bit pointer
     * \param len    Move distance. \f$ len \in [0..64] \f$
     * \sa move_left
     */
	SDSL_CONSTEXPR static void move_right(const uint64_t*& word, uint8_t& offset, const uint8_t len);

	//! Move the bit-pointer (=uint64_t word and offset) `len` to the left.
	/*!\param word   64-bit word part of the bit pointer
     * \param offset Offset part of the bit pointer
     * \param len    Move distance. \f$ len \in [0..64] \f$
     * \sa move_right
     */
	SDSL_CONSTEXPR static void move_left(const uint64_t*& word, uint8_t& offset, const uint8_t len);

	//! Get the first one bit in the interval \f$[idx..\infty )\f$
	SDSL_CONSTEXPR static uint64_t next(const uint64_t* word, uint64_t idx);

	//! Get the one bit with the greatest position in the interval \f$[0..idx]\f$
	SDSL_CONSTEXPR static uint64_t prev(const uint64_t* word, uint64_t idx);

	//! reverses a given 64 bit word
	SDSL_CONSTEXPR static uint64_t rev(uint64_t x);
};


// ============= inline - implementations ================

// see page 11, Knuth TAOCP Vol 4 F1A
template <typename T>
SDSL_CONSTEXPR inline uint64_t bits_impl<T>::cnt(uint64_t x)
{
#ifdef __SSE4_2__
	return __builtin_popcountll(x);
#else
#ifdef POPCOUNT_TL
	return lt_cnt[x & 0xFFULL] + lt_cnt[(x >> 8) & 0xFFULL] + lt_cnt[(x >> 16) & 0xFFULL] +
		   lt_cnt[(x >> 24) & 0xFFULL] + lt_cnt[(x >> 32) & 0xFFULL] + lt_cnt[(x >> 40) & 0xFFULL] +
		   lt_cnt[(x >> 48) & 0xFFULL] + lt_cnt[(x >> 56) & 0xFFULL];
#else
	x = x - ((x >> 1) & 0x5555555555555555ull);
	x = (x & 0x3333333333333333ull) + ((x >> 2) & 0x3333333333333333ull);
	x = (x + (x >> 4)) & 0x0f0f0f0f0f0f0f0full;
	return (0x0101010101010101ull * x >> 56);
#endif
#endif
}

template <typename T>
SDSL_CONSTEXPR inline uint32_t bits_impl<T>::cnt32(uint32_t x)
{
	x = x - ((x >> 1) & 0x55555555);
	x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
	return (0x10101010 * x >> 28) + (0x01010101 * x >> 28);
}


template <typename T>
SDSL_CONSTEXPR inline uint32_t bits_impl<T>::cnt11(uint64_t x, uint64_t& c)
{
	// extract "11" 2bit blocks
	uint64_t ex11 = (x & (x >> 1)) & 0x5555555555555555ULL, t{};
	// extract "10" 2bit blocks
	uint64_t ex10or01 = (ex11 | (ex11 << 1)) ^ x;

	x = ex11 | ((t = (ex11 | (ex11 << 1)) + (((ex10or01 << 1) & 0x5555555555555555ULL) | c)) &
				(ex10or01 & 0x5555555555555555ULL));
	c = (ex10or01 >> 63) or (t < (ex11 | (ex11 << 1)));

	x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
	x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
	return (0x0101010101010101ULL * x >> 56);
}

template <typename T>
SDSL_CONSTEXPR inline uint32_t bits_impl<T>::cnt11(uint64_t x)
{
	// extract "11" 2bit blocks
	uint64_t ex11 = (x & (x >> 1)) & 0x5555555555555555ULL;
	// extract "10" 2bit blocks
	uint64_t ex10or01 = (ex11 | (ex11 << 1)) ^ x;

	x = ex11 | (((ex11 | (ex11 << 1)) + ((ex10or01 << 1) & 0x5555555555555555ULL)) &
				(ex10or01 & 0x5555555555555555ULL));

	x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
	x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
	return (0x0101010101010101ULL * x >> 56);
}

template <typename T>
SDSL_CONSTEXPR inline uint32_t bits_impl<T>::cnt10(uint64_t x, uint64_t& c)
{
	uint32_t res = cnt((x ^ ((x << 1) | c)) & (~x));
	c			 = (x >> 63);
	return res;
}

template <typename T>
SDSL_CONSTEXPR inline uint64_t bits_impl<T>::map10(uint64_t x, uint64_t c)
{
	return ((x ^ ((x << 1) | c)) & (~x));
}

template <typename T>
SDSL_CONSTEXPR inline uint32_t bits_impl<T>::cnt01(uint64_t x, uint64_t& c)
{
	uint32_t res = cnt((x ^ ((x << 1) | c)) & x);
	c			 = (x >> 63);
	return res;
}

template <typename T>
SDSL_CONSTEXPR inline uint64_t bits_impl<T>::map01(uint64_t x, uint64_t c)
{
	return ((x ^ ((x << 1) | c)) & x);
}

template <typename T>
SDSL_CONSTEXPR inline uint32_t bits_impl<T>::sel(uint64_t x, uint32_t i)
{
#ifdef __BMI2__
	// taken from folly
	return _tzcnt_u64(_pdep_u64(1ULL << (i - 1), x));
#endif
#ifdef __SSE4_2__
	uint64_t s = x, b{};
	s		   = s - ((s >> 1) & 0x5555555555555555ULL);
	s		   = (s & 0x3333333333333333ULL) + ((s >> 2) & 0x3333333333333333ULL);
	s		   = (s + (s >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
	s		   = 0x0101010101010101ULL * s;
	// now s contains 8 bytes s[7],...,s[0]; s[j] contains the cumulative sum
	// of (j+1)*8 least significant bits of s
	b = (s + ps_overflow[i]) & 0x8080808080808080ULL;
	// ps_overflow contains a bit mask x consisting of 8 bytes
	// x[7],...,x[0] and x[j] is set to 128-j
	// => a byte b[j] in b is >= 128 if cum sum >= j

	// __builtin_ctzll returns the number of trailing zeros, if b!=0
	int byte_nr = __builtin_ctzll(b) >> 3; // byte nr in [0..7]
	s <<= 8;
	i -= (s >> (byte_nr << 3)) & 0xFFULL;
	return (byte_nr << 3) + lt_sel[((i - 1) << 8) + ((x >> (byte_nr << 3)) & 0xFFULL)];
#endif
	return _sel(x, i);
}

template <typename T>
SDSL_CONSTEXPR inline uint32_t bits_impl<T>::_sel(uint64_t x, uint32_t i)
{
	uint64_t s = x, b{}; // s = sum
	s		   = s - ((s >> 1) & 0x5555555555555555ULL);
	s		   = (s & 0x3333333333333333ULL) + ((s >> 2) & 0x3333333333333333ULL);
	s		   = (s + (s >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
	s		   = 0x0101010101010101ULL * s;
	b		   = (s + ps_overflow
			 [i]); //&0x8080808080808080ULL;// add something to the partial sums to cause overflow
	i = (i - 1) << 8;
	if (b & 0x0000000080000000ULL)	 // byte <=3
		if (b & 0x0000000000008000ULL) //byte <= 1
			if (b & 0x0000000000000080ULL)
				return lt_sel[(x & 0xFFULL) + i];
			else
				return 8 + lt_sel[(((x >> 8) & 0xFFULL) + i - ((s & 0xFFULL) << 8)) &
								  0x7FFULL];											  //byte 1;
		else																			  //byte >1
		if (b & 0x0000000000800000ULL)													  //byte <=2
			return 16 + lt_sel[(((x >> 16) & 0xFFULL) + i - (s & 0xFF00ULL)) & 0x7FFULL]; //byte 2;
		else
			return 24 + lt_sel[(((x >> 24) & 0xFFULL) + i - ((s >> 8) & 0xFF00ULL)) &
							   0x7FFULL]; //byte 3;
	else								  //  byte > 3
	if (b & 0x0000800000000000ULL)		  // byte <=5
		if (b & 0x0000008000000000ULL)	//byte <=4
			return 32 + lt_sel[(((x >> 32) & 0xFFULL) + i - ((s >> 16) & 0xFF00ULL)) &
							   0x7FFULL]; //byte 4;
		else
			return 40 + lt_sel[(((x >> 40) & 0xFFULL) + i - ((s >> 24) & 0xFF00ULL)) &
							   0x7FFULL]; //byte 5;
	else								  // byte >5
	if (b & 0x0080000000000000ULL)		  //byte<=6
		return 48 +
			   lt_sel[(((x >> 48) & 0xFFULL) + i - ((s >> 32) & 0xFF00ULL)) & 0x7FFULL]; //byte 6;
	else
		return 56 +
			   lt_sel[(((x >> 56) & 0xFFULL) + i - ((s >> 40) & 0xFF00ULL)) & 0x7FFULL]; //byte 7;
	return 0;
}

// using built-in method or
// 64-bit version of 32-bit proposal of
// http://www-graphics.stanford.edu/~seander/bithacks.html
template <typename T>
SDSL_CONSTEXPR inline uint32_t bits_impl<T>::hi(uint64_t x)
{
#ifdef __SSE4_2__
	if (x == 0) return 0;
	return 63 - __builtin_clzll(x);
#else
	uint64_t t{}, tt{};			  // temporaries
	if ((tt = x >> 32)) {	 // hi >= 32
		if ((t = tt >> 16)) { // hi >= 48
			return (tt = t >> 8) ? 56 + lt_hi[tt] : 48 + lt_hi[t];
		} else { // hi < 48
			return (t = tt >> 8) ? 40 + lt_hi[t] : 32 + lt_hi[tt];
		}
	} else {				 // hi < 32
		if ((t = x >> 16)) { // hi >= 16
			return (tt = t >> 8) ? 24 + lt_hi[tt] : 16 + lt_hi[t];
		} else { // hi < 16
			return (tt = x >> 8) ? 8 + lt_hi[tt] : lt_hi[x];
		}
	}
#endif
}

// details see: http://citeseer.ist.psu.edu/leiserson98using.html
// or page 10, Knuth TAOCP Vol 4 F1A
template <typename T>
SDSL_CONSTEXPR inline uint32_t bits_impl<T>::lo(uint64_t x)
{
#ifdef __SSE4_2__
	if (x == 0) return 0;
	return __builtin_ctzll(x);
#else
	if (x & 1) return 0;
	if (x & 3) return 1;
	if (x & 7) return 2;
	if (x & 0x7FF) { // in average every second random number x can be answered this way
		return lt_lo[(x & 0x7FF) >> 3] + 3;
	}
	// x&-x equals x with only the lsb set
	return lt_deBruijn_to_idx[((x & -x) * deBruijn64) >> 58];
#endif
}

template <typename T>
SDSL_CONSTEXPR inline uint32_t bits_impl<T>::hi11(uint64_t x)
{
	// extract "11" 2bit blocks
	uint64_t ex11 = (x & (x >> 1)) & 0x5555555555555555ULL;
	// extract "10" 2bit blocks
	uint64_t ex10or01 = (ex11 | (ex11 << 1)) ^ x;
	// extract "10" 2bit blocks
	ex11 += (((ex11 | (ex11 << 1)) + ((ex10or01 << 1) & 0x5555555555555555ULL)) &
			 ((ex10or01 & 0x5555555555555555ULL) | ex11));
	return hi(ex11);
}


template <typename T>
SDSL_CONSTEXPR inline uint32_t bits_impl<T>::sel11(uint64_t x, uint32_t i, uint32_t c)
{
	uint64_t ex11	 = (x & (x >> 1)) & 0x5555555555555555ULL;
	uint64_t ex10or01 = (ex11 | (ex11 << 1)) ^ x;
	ex11 += (((ex11 | (ex11 << 1)) + (((ex10or01 << 1) & 0x5555555555555555ULL) | c)) &
			 ((ex10or01 & 0x5555555555555555ULL) | ex11));
	return sel(ex11, i);
}

template <typename T>
SDSL_CONSTEXPR inline void bits_impl<T>::write_int(uint64_t* word, uint64_t x, uint8_t offset, const uint8_t len)
{
	x &= bits_impl<T>::lo_set[len];
	if (offset + len < 64) {
		*word &= ((bits_impl<T>::all_set << (offset + len)) |
				  bits_impl<T>::lo_set[offset]); // mask 1..10..01..1
		*word |= (x << offset);
		//		*word ^= ((*word ^ x) & (bits_impl<T>::lo_set[len] << offset) );
		//      surprisingly the above line is slower than the lines above
	} else {
		*word &= ((bits_impl<T>::lo_set[offset])); // mask 0....01..1
		*word |= (x << offset);
		if ((offset = (offset + len) & 0x3F)) {				// offset+len > 64
			*(word + 1) &= (~bits_impl<T>::lo_set[offset]); // mask 1...10..0
			//			*(word+1) &= bits_impl<T>::lo_unset[offset]; // mask 1...10..0
			//          surprisingly the above line is slower than the line above
			*(word + 1) |= (x >> (len - offset));
		}
	}
}

template <typename T>
SDSL_CONSTEXPR inline void
bits_impl<T>::write_int_and_move(uint64_t*& word, uint64_t x, uint8_t& offset, const uint8_t len)
{
	x &= bits_impl<T>::lo_set[len];
	if (offset + len < 64) {
		*word &= ((bits_impl<T>::all_set << (offset + len)) |
				  bits_impl<T>::lo_set[offset]); // mask 1..10..01..1
		*word |= (x << offset);
		offset += len;
	} else {
		*word &= ((bits_impl<T>::lo_set[offset])); // mask 0....01..1
		*word |= (x << offset);
		if ((offset = (offset + len)) > 64) { // offset+len >= 64
			offset &= 0x3F;
			*(++word) &= (~bits_impl<T>::lo_set[offset]); // mask 1...10..0
			*word |= (x >> (len - offset));
		} else {
			offset = 0;
			++word;
		}
	}
}

template <typename T>
SDSL_CONSTEXPR inline uint64_t bits_impl<T>::read_int(const uint64_t* word, uint8_t offset, const uint8_t len)
{
	uint64_t w1 = (*word) >> offset;
	if ((offset + len) > 64) {												// if offset+len > 64
		return w1 |															// w1 or w2 adepted:
			   ((*(word + 1) & bits_impl<T>::lo_set[(offset + len) & 0x3F]) // set higher bits zero
				<< (64 - offset));											// move bits to the left
	} else {
		return w1 & bits_impl<T>::lo_set[len];
	}
}

template <typename T>
SDSL_CONSTEXPR inline uint64_t
bits_impl<T>::read_int_and_move(const uint64_t*& word, uint8_t& offset, const uint8_t len)
{
	uint64_t w1 = (*word) >> offset;
	if ((offset = (offset + len)) >= 64) { // if offset+len > 64
		if (offset == 64) {
			offset &= 0x3F;
			++word;
			return w1;
		} else {
			offset &= 0x3F;
			return w1 | (((*(++word)) & bits_impl<T>::lo_set[offset]) << (len - offset));
		}
	} else {
		return w1 & bits_impl<T>::lo_set[len];
	}
}

template <typename T>
SDSL_CONSTEXPR inline uint64_t bits_impl<T>::read_unary(const uint64_t* word, uint8_t offset)
{
	uint64_t w = *word >> offset;
	if (w) {
		return bits_impl<T>::lo(w);
	} else {
		if (0 != (w  = *(++word))) return bits_impl<T>::lo(w) + 64 - offset;
		uint64_t cnt = 2;
		while (0 == (w = *(++word)))
			++cnt;
		return bits_impl<T>::lo(w) + (cnt << 6) - offset;
	}
	return 0;
}

template <typename T>
SDSL_CONSTEXPR inline uint64_t bits_impl<T>::read_unary_and_move(const uint64_t*& word, uint8_t& offset)
{
	uint64_t w = (*word) >> offset; // temporary variable is good for the performance
	if (w) {
		uint8_t r = bits_impl<T>::lo(w);
		offset	= (offset + r + 1) & 0x3F;
		// we know that offset + r +1 <= 64, so if the new offset equals 0 increase word
		word += (offset == 0);
		return r;
	} else {
		uint8_t rr = 0;
		if (0 != (w = *(++word))) {
			rr	 = bits_impl<T>::lo(w) + 64 - offset;
			offset = (offset + rr + 1) & 0x3F;
			word += (offset == 0);
			return rr;
		} else {
			uint64_t cnt_1 = 1;
			while (0 == (w = *(++word)))
				++cnt_1;
			rr	 = bits_impl<T>::lo(w) + 64 - offset;
			offset = (offset + rr + 1) & 0x3F;
			word += (offset == 0);
			return ((cnt_1) << 6) + rr;
		}
	}
	return 0;
}

template <typename T>
SDSL_CONSTEXPR inline void bits_impl<T>::move_right(const uint64_t*& word, uint8_t& offset, const uint8_t len)
{
	if ((offset += len) & 0xC0) { // if offset >= 65
		offset &= 0x3F;
		++word;
	}
}

template <typename T>
SDSL_CONSTEXPR inline void bits_impl<T>::move_left(const uint64_t*& word, uint8_t& offset, const uint8_t len)
{
	if ((offset -= len) & 0xC0) { // if offset-len<0
		offset &= 0x3F;
		--word;
	}
}

template <typename T>
SDSL_CONSTEXPR inline uint64_t bits_impl<T>::next(const uint64_t* word, uint64_t idx)
{
	word += (idx >> 6);
	if (*word & ~lo_set[idx & 0x3F]) {
		return (idx & ~((size_t)0x3F)) + lo(*word & ~lo_set[idx & 0x3F]);
	}
	idx = (idx & ~((size_t)0x3F)) + 64;
	++word;
	while (*word == 0) {
		idx += 64;
		++word;
	}
	return idx + lo(*word);
}

template <typename T>
SDSL_CONSTEXPR inline uint64_t bits_impl<T>::prev(const uint64_t* word, uint64_t idx)
{
	word += (idx >> 6);
	if (*word & lo_set[(idx & 0x3F) + 1]) {
		return (idx & ~((size_t)0x3F)) + hi(*word & lo_set[(idx & 0x3F) + 1]);
	}
	idx = (idx & ~((size_t)0x3F)) - 64;
	--word;
	while (*word == 0) {
		idx -= 64;
		--word;
	}
	return idx + hi(*word);
}

template <typename T>
SDSL_CONSTEXPR inline uint64_t bits_impl<T>::rev(uint64_t x)
{
	x = ((x & 0x5555555555555555ULL) << 1) | ((x & 0xAAAAAAAAAAAAAAAAULL) >> 1);
	x = ((x & 0x3333333333333333ULL) << 2) | ((x & 0xCCCCCCCCCCCCCCCCULL) >> 2);
	x = ((x & 0x0F0F0F0F0F0F0F0FULL) << 4) | ((x & 0xF0F0F0F0F0F0F0F0ULL) >> 4);
	x = ((x & 0x00FF00FF00FF00FFULL) << 8) | ((x & 0xFF00FF00FF00FF00ULL) >> 8);
	x = ((x & 0x0000FFFF0000FFFFULL) << 16) | ((x & 0xFFFF0000FFFF0000ULL) >> 16);
	x = ((x & 0x00000000FFFFFFFFULL) << 32) | ((x & 0xFFFFFFFF00000000ULL) >> 32);
	return x;
}

template <typename T>
constexpr uint8_t bits_impl<T>::lt_cnt[256];
template <typename T>
constexpr uint32_t bits_impl<T>::lt_deBruijn_to_idx[64];
template <typename T>
constexpr uint32_t bits_impl<T>::lt_hi[256];
template <typename T>
constexpr uint64_t bits_impl<T>::lo_set[65];
template <typename T>
constexpr uint64_t bits_impl<T>::lo_unset[65];
template <typename T>
constexpr uint64_t bits_impl<T>::ps_overflow[65];
template <typename T>
constexpr uint8_t bits_impl<T>::lt_sel[256 * 8];
template <typename T>
constexpr uint64_t bits_impl<T>::lt_fib[92];
template <typename T>
constexpr uint8_t bits_impl<T>::lt_lo[256];

using bits = bits_impl<>;


} // end namespace sdsl

#endif
