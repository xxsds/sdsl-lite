// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
/*
 *   Copyright (c) 2014 Juha Karkkainen, Dominik Kempa and Simon J. Puglisi
 *
 *   Permission is hereby granted, free of charge, to any person
 *   obtaining a copy of this software and associated documentation
 *   files (the "Software"), to deal in the Software without
 *   restriction, including without limitation the rights to use,
 *   copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the
 *   Software is furnished to do so, subject to the following
 *   conditions:
 *
 *   The above copyright notice and this permission notice shall be
 *   included in all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 *   HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *   OTHER DEALINGS IN THE SOFTWARE.
 *
 *   Simon Gog made the following changes:
 *     - replace std::vectors by int_vectors
 *     - add support for rank0
 *     - added naive implementation of method get_int
 *     - added the initial implementation of select
 */
#ifndef INCLUDED_SDSL_HYB_VECTOR
#define INCLUDED_SDSL_HYB_VECTOR

#include <algorithm>
#include <bit>
#include <assert.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <span>
#include <ranges>
#include <type_traits>

#if defined(__AVX2__)
#include <immintrin.h>
#endif

#include <sdsl/bit_vector_helper.hpp>
#include <sdsl/bits.hpp>
#include <sdsl/cereal.hpp>
#include <sdsl/int_vector.hpp>
#include <sdsl/io.hpp>
#include <sdsl/iterators.hpp>
#include <sdsl/structure_tree.hpp>
#include <sdsl/util.hpp>

namespace sdsl
{

// Needed for friend declarations.
template <uint8_t t_b = 1, uint32_t k_sb_rate = 16>
class rank_support_hyb;
template <uint8_t t_b = 1, uint32_t k_sb_rate = 16>
class select_support_hyb;

//! A hybrid-encoded compressed bitvector representation
/*!
 * \tparam k_sblock_rate  Superblock rate (number of blocks inside superblock)
 *
 * References:
 *  - Juha Karkkainen, Dominik Kempa and Simon J. Puglisi.
 *    Hybrid Compression of Bitvectors for the FM-Index.
 *    DCC 2014.
 */
template <uint32_t k_sblock_rate = 16>
class hyb_vector
{
public:
    typedef bit_vector::size_type size_type;
    typedef bit_vector::value_type value_type;
    typedef bit_vector::difference_type difference_type;
    typedef random_access_const_iterator<hyb_vector> iterator;
    typedef rank_support_hyb<1, k_sblock_rate> rank_1_type;
    typedef rank_support_hyb<0, k_sblock_rate> rank_0_type;
    typedef select_support_hyb<1, k_sblock_rate> select_1_type;
    typedef select_support_hyb<0, k_sblock_rate> select_0_type;

    friend class rank_support_hyb<1, k_sblock_rate>;
    friend class rank_support_hyb<0, k_sblock_rate>;
    friend class select_support_hyb<1, k_sblock_rate>;
    friend class select_support_hyb<0, k_sblock_rate>;

private:
    static constexpr uint32_t k_block_size = 256;
    static constexpr uint32_t k_block_bytes = k_block_size / 8;
    static constexpr uint32_t k_sblock_prefix_size = 2 * sizeof(uint32_t);
    static constexpr uint32_t k_sblock_header_size = k_sblock_prefix_size + sizeof(uint16_t) * k_sblock_rate;
    static constexpr uint32_t k_sblock_size = k_block_size * k_sblock_rate;
    // A hyperblock spans at most 2^31 bits and a whole number of superblocks.
    static constexpr uint32_t k_hblock_rate = (uint32_t{1} << 31) / k_block_size / k_sblock_rate * k_sblock_rate;
    // A superblock header starts with a 30-bit payload offset relative to its hyperblock; bit 31 marks a
    // superblock whose blocks are all zeros or all ones.
    static constexpr uint32_t k_relative_offset_bits = 30;
    static constexpr uint32_t k_uniform_mask = uint32_t{1} << 31;
    // A 16-bit block descriptor holds the block population (0 to k_block_size) in its low bits, then a special
    // bit, then the payload size in bytes.
    static constexpr uint32_t k_population_bits = std::bit_width(k_block_size);
    static constexpr uint32_t k_population_mask = bits::lo_set[k_population_bits];
    static constexpr uint32_t k_special_bit_shift = k_population_bits;
    static constexpr uint32_t k_special_bit_mask = uint32_t{1} << k_special_bit_shift;
    static constexpr uint32_t k_payload_size_shift = k_special_bit_shift + 1;

    size_type m_size = 0;           // original bitvector size
    int_vector<8> m_trunk;          // body of encoded blocks
    int_vector<8> m_sblock_header;  // sblock headers
    int_vector<64> m_hblock_header; // hblock headers

    //! Reads a 64-bit word from m_trunk, where encoded blocks start at arbitrary byte offsets.
    static uint64_t read_trunk_word(uint8_t const * ptr)
    {
        uint64_t word;
        std::memcpy(&word, ptr, sizeof(word));
        return word;
    }


public:
    //! Default constructor
    hyb_vector() = default;
    hyb_vector(hyb_vector const & hybrid) = default;
    hyb_vector(hyb_vector && hybrid) = default;
    hyb_vector & operator=(hyb_vector const & hybrid) = default;
    hyb_vector & operator=(hyb_vector && hybrid) = default;

    //! Constructor
    hyb_vector(bit_vector const & bv)
    {
        m_size = bv.size();

        // Compute the number of blocks.
        size_type n_blocks = (m_size + k_block_size - 1) / k_block_size;
        size_type n_sblocks = (n_blocks + k_sblock_rate - 1) / k_sblock_rate;
        size_type n_hblocks = (n_blocks + k_hblock_rate - 1) / k_hblock_rate;

        size_type trunk_size = 0;

        // runs_lookup[i] = number of runs - 1 in the binary encoding of i.
        int_vector<8> runs_lookup(65536, 0);
        runs_lookup[0] = 0;
        for (uint32_t i = 1; i < 65536; ++i)
        {
            runs_lookup[i] = runs_lookup[i >> 1];
            if (i >= 32768)
                --runs_lookup[i];
            if ((i & 1) != ((i >> 1) & 1))
                ++runs_lookup[i];
        }

        // Compute optimal encoding for each block.
        uint64_t const * bv_ptr = bv.data();
        for (size_type block_id = 0; block_id < n_blocks; ++block_id)
        {
            size_type block_beg = block_id * k_block_size;
            size_type block_end = block_beg + k_block_size;

            uint32_t ones = 0;
            uint32_t runs = 0;

            if (block_end <= m_size)
            {
                // Count the number of ones, fast.
                uint64_t const * ptr64 = bv_ptr;
                for (uint8_t i = 0; i < 4; ++i)
                    ones += bits::cnt(*ptr64++);

                // Count the number of runs, fast.
                ptr64 = bv_ptr;
                for (uint8_t i = 0; i < 4; ++i)
                {
                    // Count changes of bits inside 16-bit words of *ptr64.
                    for (uint8_t j = 0; j < 4; ++j)
                        runs += runs_lookup[((*ptr64) >> (16 * j)) & 0xffff];

                    // Count changes of bits between 16-bit words of *ptr64.
                    for (uint8_t j = 0; j < 3; ++j)
                        runs += ((((*ptr64) >> (16 * j + 15)) & 1) ^ (((*ptr64) >> (16 * j + 16)) & 1));
                    ++ptr64;
                }

                // Count changes of bits between 64-bit words.
                ptr64 = bv_ptr;
                for (uint8_t i = 0; i < 3; ++i)
                {
                    runs += ((((*ptr64) >> 63) & 1) ^ ((*(ptr64 + 1)) & 1));
                    ++ptr64;
                }
                ++runs;
            }
            else
            {
                // Count number of ones and runs, slow.
                uint8_t prevbit = 2;
                for (size_type i = block_beg; i < block_end; ++i)
                {
                    uint8_t bit = (i < m_size ? bv[i] : 0);
                    if (bit == 1)
                        ++ones;
                    if (bit != prevbit)
                        ++runs;
                    prevbit = bit;
                }
            }

            // Choose best encoding.
            uint32_t minority_enc_size = std::min(ones, k_block_size - ones);
            uint32_t runs_enc_size = (uint32_t)std::max(0, (int32_t)runs - 2);
            uint32_t best_enc_size = std::min(minority_enc_size, runs_enc_size);
            best_enc_size = std::min(best_enc_size, k_block_bytes);

            // Update the answer.
            trunk_size += best_enc_size;
            bv_ptr += k_block_size / 64;
        }

        // Allocate the memory.
        m_sblock_header = int_vector<8>(n_sblocks * k_sblock_header_size, 0);
        m_hblock_header = int_vector<64>(n_hblocks * 2, 0);
        m_trunk = int_vector<8>(trunk_size, 0);

        // The actual encoding follows.
        size_type tot_rank = 0;    // stores current rank value
        size_type sblock_ones = 0; // number of 1s inside superblock
        size_type trunk_ptr = 0;

        // Process blocks left to right.
        bv_ptr = bv.data();
        for (size_type block_id = 0; block_id < n_blocks; ++block_id)
        {
            size_type block_beg = block_id * k_block_size;
            size_type block_end = block_beg + k_block_size;
            size_type sblock_id = block_id / k_sblock_rate;
            size_type hblock_id = block_id / k_hblock_rate;
            std::span<uint8_t> header{&m_sblock_header[sblock_id * k_sblock_header_size],
                                     k_sblock_header_size};

            // Update hblock header.
            if (!(block_id % k_hblock_rate))
            {
                auto [payload_offset, ones_before] = hyperblock_header(hblock_id);
                payload_offset = trunk_ptr;
                ones_before = tot_rank;
            }

            // Update sblock header.
            if (!(block_id % k_sblock_rate))
            {
                auto [payload_offset, ones_before] = hyperblock_header(hblock_id);
                detail::write_unaligned<uint32_t>(header, trunk_ptr - payload_offset);
                detail::write_unaligned<uint32_t>(header, tot_rank - ones_before, 1);

                // If the sblock is uniform, flip the bit.
                if (sblock_id && (!sblock_ones || sblock_ones == k_sblock_size))
                {
                    std::span<uint8_t> previous{&m_sblock_header[(sblock_id - 1) * k_sblock_header_size],
                                               k_sblock_header_size};
                    auto packed = detail::read_unaligned<uint32_t>(previous);
                    detail::write_unaligned(previous, packed | k_uniform_mask);
                }

                // Reset the number of ones in sblock.
                sblock_ones = 0;
            }

            uint32_t ones = 0;
            uint32_t runs = 0;

            // Compute the number of 1-bits and runs inside current block.
            if (block_end <= m_size)
            {
                // Count the number of ones, fast.
                uint64_t const * ptr64 = bv_ptr;
                for (uint8_t i = 0; i < 4; ++i)
                    ones += bits::cnt(*ptr64++);

                // Count the number of runs, fast.
                ptr64 = bv_ptr;
                for (uint8_t i = 0; i < 4; ++i)
                {
                    for (uint8_t j = 0; j < 4; ++j)
                        runs += runs_lookup[((*ptr64) >> (16 * j)) & 0xffff];
                    for (uint8_t j = 0; j < 3; ++j)
                        runs += ((((*ptr64) >> (16 * j + 15)) & 1) ^ (((*ptr64) >> (16 * j + 16)) & 1));
                    ++ptr64;
                }
                ptr64 = bv_ptr;
                for (uint8_t i = 0; i < 3; ++i)
                {
                    runs += ((((*ptr64) >> 63) & 1) ^ ((*(ptr64 + 1)) & 1));
                    ++ptr64;
                }
                ++runs;
            }
            else
            {
                // Count number of ones and runs, slow.
                uint8_t prevbit = 2;
                for (size_type i = block_beg; i < block_end; ++i)
                {
                    uint8_t bit = (i < m_size ? bv[i] : 0);
                    if (bit == 1)
                        ++ones;
                    if (bit != prevbit)
                        ++runs;
                    prevbit = bit;
                }
            }
            uint32_t zeros = k_block_size - ones;

            // Store block popcount.
            uint16_t descriptor = ones;
            if (ones == k_block_size)
                descriptor |= k_special_bit_mask;

            if (0 < ones && ones < k_block_size)
            { // non uniform block
                uint32_t minority_enc_size = std::min(ones, zeros);
                uint32_t runs_enc_size = (uint32_t)std::max(0, (int32_t)runs - 2);
                uint32_t best_enc_size = std::min(minority_enc_size, runs_enc_size);

                if (k_block_bytes <= best_enc_size)
                {
                    // Use plain encoding.
                    descriptor |= (k_block_bytes << k_payload_size_shift);

                    // Copy original 256 bits from bv into trunk.
                    if (block_end <= m_size)
                    {
                        std::memcpy(((uint8_t *)m_trunk.data()) + trunk_ptr, bv_ptr, k_block_bytes);
                        trunk_ptr += k_block_bytes;
                    }
                    else
                    {
                        for (size_type i = block_beg; i < block_end; i += 64)
                        {
                            uint64_t w = 0;
                            for (size_type j = i; j < std::min(i + 64, block_end); ++j)
                            {
                                uint8_t bit = (j < m_size ? bv[j] : 0);
                                if (bit)
                                    w |= ((uint64_t)1 << (j - i));
                            }
                            std::memcpy(((uint8_t *)m_trunk.data()) + trunk_ptr, &w, sizeof(w));
                            trunk_ptr += 8;
                        }
                    }
                }
                else
                {
                    if (runs_enc_size < minority_enc_size)
                    {

                        // Use runs encoding.
                        descriptor |= (runs_enc_size << k_payload_size_shift);
                        descriptor |= (bv[block_beg] << k_special_bit_shift);

                        if (block_end <= m_size)
                        {
                            // Find run ends, fast.
                            uint32_t runid = 0;
                            uint64_t const * ptr64 = bv_ptr;

                            uint64_t w = 0;
                            for (uint8_t i = 0; runid < runs_enc_size && i < 4; ++i)
                            {
                                // Check if run end aligns with the end of the 64-bit word.
                                if (i > 0 && (w & 1) != ((*ptr64) & 1))
                                    m_trunk[trunk_ptr + runid++] = 64 * i - 1;

                                w = (*ptr64++);
                                for (uint8_t j = 0; runid < runs_enc_size && j < 63; ++j)
                                {
                                    if ((w & 1) != ((w >> 1) & 1))
                                        m_trunk[trunk_ptr + runid++] = j + i * 64;
                                    w >>= 1;
                                }
                            }
                            trunk_ptr += runid;
                        }
                        else
                        {
                            // Find run ends, slow.
                            uint8_t prevbit = 2;
                            uint32_t runid = 0;

                            for (size_type i = block_beg; runid < runs_enc_size; ++i)
                            {
                                uint8_t bit = (i < m_size ? bv[i] : 0);
                                if (bit != prevbit && i != block_beg)
                                    m_trunk[trunk_ptr + runid++] = (i - block_beg - 1);
                                prevbit = bit;
                            }
                            trunk_ptr += runid;
                        }
                    }
                    else
                    {
                        // Use minority encoding.
                        // Update sblock header.
                        descriptor |= (minority_enc_size << k_payload_size_shift);
                        if (ones < zeros)
                            descriptor |= k_special_bit_mask;
                        uint8_t keybit = (ones < zeros);

                        // Find positions of 1-bits, fast.
                        if (block_end <= m_size)
                        {
                            uint64_t const * ptr64 = bv_ptr;
                            for (uint8_t i = 0; i < 4; ++i)
                            {
                                uint64_t w = (*ptr64++);
                                for (uint8_t j = 0; j < 64; ++j)
                                {
                                    if ((w & 1) == keybit)
                                        m_trunk[trunk_ptr++] = j + 64 * i;
                                    w >>= 1;
                                }
                            }
                        }
                        else
                        {
                            for (size_type i = block_beg; i < block_end; ++i)
                            {
                                uint8_t bit = (i < m_size ? bv[i] : 0);

                                if (bit == keybit)
                                    m_trunk[trunk_ptr++] = i - block_beg;
                            }
                        }
                    }
                }
            }

            detail::write_unaligned(header, descriptor,
                                    k_sblock_prefix_size / sizeof(descriptor) + block_id % k_sblock_rate);

            // Update global rank.
            tot_rank += ones;
            sblock_ones += ones;
            bv_ptr += k_block_size / 64;
        }
    }

private:
    struct superblock_view
    {
        uint32_t relative_offset;
        uint32_t relative_ones;
        bool uniform;
        std::span<uint8_t const> descriptors;
    };

    superblock_view superblock_header(size_type sblock) const
    {
        std::span<uint8_t const> bytes{&m_sblock_header[sblock * k_sblock_header_size],
                                      k_sblock_header_size};
        auto packed = detail::read_unaligned<uint32_t>(bytes);
        return {static_cast<uint32_t>(packed & bits::lo_set[k_relative_offset_bits]),
                detail::read_unaligned<uint32_t>(bytes, 1),
                (packed & k_uniform_mask) != 0, bytes.subspan(k_sblock_prefix_size)};
    }

    template <typename T>
    struct hyperblock_reference
    {
        T & payload_offset;
        T & ones_before;
    };

    hyperblock_reference<uint64_t> hyperblock_header(size_type hblock)
    {
        return {m_hblock_header[2 * hblock], m_hblock_header[2 * hblock + 1]};
    }

    hyperblock_reference<uint64_t const> hyperblock_header(size_type hblock) const
    {
        return {m_hblock_header[2 * hblock], m_hblock_header[2 * hblock + 1]};
    }

    template <typename T>
    struct block_descriptor
    {
        T payload_bytes;
        T ones;
        // First run's bit for run encoding; minority bit for position encoding.
        using bit_type = std::conditional_t<std::is_integral_v<T>, bool, T>;
        bit_type special_bit;
    };

    template <typename T>
    static block_descriptor<T> decode_block(T const & packed)
    {
        return {packed >> k_payload_size_shift, packed & k_population_mask,
                static_cast<typename block_descriptor<T>::bit_type>((packed >> k_special_bit_shift) & 1)};
    }

#if defined(__AVX2__)
    // Favor the default rate's shorter reduction; widen only for larger superblocks.
    static constexpr uint32_t k_descriptor_lanes = k_sblock_rate >= 64 ? 16 : 8;
    // sum_lanes reads each lane as a signed 16-bit value (PMADDWD).
    static constexpr uint32_t k_lane_sum_limit = INT16_MAX;
#else
    static constexpr uint32_t k_descriptor_lanes = 8;
    // Lanes accumulate in uint16_t.
    static constexpr uint32_t k_lane_sum_limit = UINT16_MAX;
#endif
    using lanes128 [[gnu::vector_size(16)]] = uint16_t;
    using lanes256 [[gnu::vector_size(32)]] = uint16_t;
    using descriptor_vector = std::conditional_t<k_descriptor_lanes == 8, lanes128, lanes256>;

    static uint32_t sum_lanes(descriptor_vector values)
    {
#if defined(__AVX2__)
        // PMADDWD widens and adds lane pairs in one step. A plain loop over the lanes also vectorizes, but with
        // extra widening shuffles, and made select 3-16% and rank up to 8% slower with GCC 16 and Clang 23.
        // Each lane sums at most 64 nine-bit populations, fitting signed 16 bits.
        __m128i sums;
        if constexpr (k_descriptor_lanes == 16)
        {
            auto pairs = _mm256_madd_epi16(std::bit_cast<__m256i>(values), _mm256_set1_epi16(1));
            sums = _mm_add_epi32(_mm256_castsi256_si128(pairs), _mm256_extracti128_si256(pairs, 1));
        }
        else
            sums = _mm_madd_epi16(std::bit_cast<__m128i>(values), _mm_set1_epi16(1));
        sums = _mm_add_epi32(sums, _mm_srli_si128(sums, 8));
        sums = _mm_add_epi32(sums, _mm_srli_si128(sums, 4));
        return static_cast<uint32_t>(_mm_cvtsi128_si32(sums));
#else
        uint32_t sum = 0;
        for (uint32_t lane = 0; lane < k_descriptor_lanes; ++lane)
            sum += values[lane];
        return sum;
#endif
    }

    struct block_location
    {
        size_type offset;
        size_type ones_before;
        block_descriptor<uint32_t> descriptor;
    };

    [[gnu::always_inline]] // Specialize the locator for the fields its caller uses.
    block_location locate_block(size_type block_id) const
    {
        auto sblock = block_id / k_sblock_rate;
        auto hblock = block_id / k_hblock_rate;
        auto [relative_offset, relative_ones, uniform, descriptors] = superblock_header(sblock);
        auto [base_offset, base_ones] = hyperblock_header(hblock);
        block_location result{base_offset + relative_offset, base_ones + relative_ones, {}};
        auto preceding = block_id % k_sblock_rate;
        if (uniform)
        {
            result.descriptor = decode_block<uint32_t>(detail::read_unaligned<uint16_t>(descriptors));
            result.ones_before += result.descriptor.ones * preceding;
            return result;
        }
        if constexpr (k_sblock_rate >= k_descriptor_lanes && k_sblock_rate <= 1024)
        {
            descriptor_vector indices{};
            for (uint32_t lane = 0; lane < k_descriptor_lanes; ++lane)
                indices[lane] = lane;
            descriptor_vector sizes{}, populations{};
            // Whole chunks only: a load never reads past this superblock's descriptors.
            constexpr size_type full = k_sblock_rate / k_descriptor_lanes * k_descriptor_lanes;
            // The queried block is never summed, so a lane sums at most ceil((rate - 1) / lanes) populations.
            constexpr size_type descriptors_per_lane = (k_sblock_rate + k_descriptor_lanes - 2) / k_descriptor_lanes;
            static_assert(descriptors_per_lane * k_block_size <= k_lane_sum_limit,
                          "Descriptor population sums overflow a 16-bit lane.");
            for (size_type j = 0; j < full; j += k_descriptor_lanes)
            {
                descriptor_vector values;
                assert(2 * j + sizeof(values) <= descriptors.size());
                std::memcpy(&values, &descriptors[2 * j], sizeof(values));
                values &=
                    static_cast<descriptor_vector>(indices + static_cast<uint16_t>(j) < static_cast<uint16_t>(preceding));
                auto decoded = decode_block(values);
                sizes += decoded.payload_bytes;
                populations += decoded.ones;
            }
            // For a rate that is not a multiple of the lane count, add one more chunk that
            // ends at the last descriptor, masking off the lanes the loop already summed.
            if constexpr (full != k_sblock_rate)
            {
                constexpr size_type last = k_sblock_rate - k_descriptor_lanes;
                descriptor_vector values;
                static_assert(2 * last + sizeof(values) == 2 * k_sblock_rate);
                std::memcpy(&values, &descriptors[2 * last], sizeof(values));
                auto lane = indices + static_cast<uint16_t>(last);
                values &= static_cast<descriptor_vector>((lane >= static_cast<uint16_t>(full))
                                                         & (lane < static_cast<uint16_t>(preceding)));
                auto decoded = decode_block(values);
                sizes += decoded.payload_bytes;
                populations += decoded.ones;
            }
            result.offset += sum_lanes(sizes);
            result.ones_before += sum_lanes(populations);
        }
        else
        {
            // Preserve the general-rate interface without partial SIMD loads
            // or overflow in the bounded 16-bit lane accumulators.
            for (size_type j = 0; j < preceding; ++j)
            {
                auto decoded = decode_block<uint32_t>(detail::read_unaligned<uint16_t>(descriptors, j));
                result.offset += decoded.payload_bytes;
                result.ones_before += decoded.ones;
            }
        }
        result.descriptor = decode_block<uint32_t>(detail::read_unaligned<uint16_t>(descriptors, preceding));
        return result;
    }

    static bool access_plain(std::span<uint8_t const, k_block_bytes> data, uint8_t position)
    {
        return detail::access_in_words(data, position);
    }

    static bool access_minority(std::span<uint8_t const> positions, bool minority_bit, uint8_t position)
    {
        return (std::ranges::find(positions, position) == positions.end()) ^ minority_bit;
    }

    static bool access_runs(std::span<uint8_t const> ends, uint32_t ones, bool first_bit, uint8_t position)
    {
        uint32_t matched_before = 0;
        uint32_t run = 0;
        uint32_t num_ends = std::min<uint32_t>(ends.size(), k_block_bytes);
        for (; run < num_ends; run += 2)
        {
            uint32_t start = run ? ends[run - 1] + 1 : 0;
            uint32_t end = ends[run] + 1;
            if (position < end)
                return (position < start) ^ first_bit;
            matched_before += end - start;
        }
        // Exactly one first-bit run remains among the two implicit final runs.
        uint32_t population = first_bit ? ones : k_block_size - ones;
        uint32_t length = population - matched_before;
        uint32_t start = run == num_ends ? ends[num_ends - 1] + 1 : k_block_size - length;
        return (position < start || position >= start + length) ^ first_bit;
    }

#if __has_cpp_attribute(gnu::always_inline)
    [[gnu::always_inline]] // Keep block decoding inside the hot query path.
#endif
    bool access_block(size_type offset, block_descriptor<uint32_t> descriptor, uint32_t position) const
    {
        auto [encoding_size, ones, special_bit] = descriptor;
        uint32_t zeros = k_block_size - ones;
        if (!encoding_size)
        {
            uint32_t first_run_length = special_bit ? ones : zeros;
            return (position >= first_run_length) ^ special_bit;
        }
        std::span<uint8_t const> data{&m_trunk[offset], encoding_size};
        if (encoding_size >= k_block_bytes)
            return access_plain(data.first<k_block_bytes>(), position);
        if (encoding_size == std::min(ones, zeros))
            return access_minority(data, special_bit, position);
        return access_runs(data, ones, special_bit, position);
    }

    uint64_t get_int_block(block_location const & location, uint32_t position, uint32_t length) const
    {
        auto [encoding_size, ones, special_bit] = location.descriptor;
        uint64_t mask = bits::lo_set[length];
        // The result's low bit represents position, even when the interval crosses a run.
        auto run_mask = [&](uint32_t start, uint32_t end) {
            start = std::clamp(start, position, position + length) - position;
            end = std::clamp(end, position, position + length) - position;
            return bits::lo_set[end] ^ bits::lo_set[start];
        };
        if (!encoding_size)
        {
            auto first_end = special_bit ? ones : k_block_size - ones;
            auto first_run = run_mask(0, first_end);
            return special_bit ? first_run : first_run ^ mask;
        }
        std::span<uint8_t const> data{&m_trunk[location.offset], encoding_size};
        if (encoding_size >= k_block_bytes)
        {
            auto word = position / 64;
            auto shift = position % 64;
            auto value = detail::read_unaligned<uint64_t>(data, word) >> shift;
            if (length > 64 - shift)
                value |= detail::read_unaligned<uint64_t>(data, word + 1) << (64 - shift);
            return value & mask;
        }
        if (encoding_size == std::min(ones, k_block_size - ones))
        {
            uint64_t value = special_bit ? 0 : mask;
            for (auto bit_position : data)
            {
                if (bit_position >= position + length)
                    break;
                if (bit_position >= position)
                    value ^= uint64_t{1} << (bit_position - position);
            }
            return value;
        }
        uint64_t first_runs = 0;
        uint32_t matched_before = 0, run = 0;
        for (; run < data.size(); run += 2)
        {
            uint32_t start = run ? data[run - 1] + 1 : 0;
            uint32_t end = data[run] + 1;
            first_runs |= run_mask(start, end);
            matched_before += end - start;
            if (end >= position + length)
                return special_bit ? first_runs : first_runs ^ mask;
        }
        // The final two run endings are implicit; one of those runs has the first bit.
        uint32_t population = special_bit ? ones : k_block_size - ones;
        uint32_t remaining = population - matched_before;
        uint32_t start = run == data.size() ? data.back() + 1 : k_block_size - remaining;
        first_runs |= run_mask(start, start + remaining);
        return special_bit ? first_runs : first_runs ^ mask;
    }

#if __has_cpp_attribute(gnu::always_inline) && !defined(__clang__)
    [[gnu::always_inline]] // GCC otherwise outlines this loop after inlining the block decoder.
#endif
    uint64_t get_int_blocks(size_type idx, uint8_t len) const
    {
        uint64_t value = 0;
        for (uint32_t copied = 0; copied < len;)
        {
            auto position = idx % k_block_size;
            auto length = std::min<uint32_t>(len - copied, k_block_size - position);
            value |= get_int_block(locate_block(idx / k_block_size), position, length) << copied;
            idx += length;
            copied += length;
        }
        return value;
    }

    uint64_t get_int_short(size_type idx, uint8_t len) const
    {
        if (!len)
            return 0;
        uint32_t position = idx % k_block_size;
        auto location = locate_block(idx / k_block_size);
        uint64_t value = 0;
        for (uint32_t bit = len; bit > 0; --bit)
        {
            value <<= 1;
            value |= access_block(location.offset, location.descriptor, position + bit - 1);
        }
        return value;
    }

public:
    //! Get the integer value of the binary string of length len starting at position idx.
    /*!\param idx Starting index of the binary representation of the integer.
     *  \param len Length of the binary representation of the integer. Default value is 64.
     *  \returns The integer value of the binary string of length len starting at position idx.
     *
     *  \pre idx+len-1 in [0..size()-1]
     *  \pre len in [1..64]
     */
#if __has_cpp_attribute(gnu::always_inline)
    [[gnu::always_inline]] // Keep the length dispatch inside the caller.
#endif
    uint64_t get_int(size_type idx, uint8_t const len = 64) const
    {
        assert(len <= 64 && idx <= m_size && len <= m_size - idx);
        if (len >= 4)
            return get_int_blocks(idx, len);
        if (len == 1)
            return (*this)[idx];
        if (idx % k_block_size + len <= k_block_size)
            return get_int_short(idx, len);
        uint64_t value = 0;
        for (size_type bit = 0; bit < len; ++bit)
        {
            value <<= 1;
            value |= (*this)[idx + len - 1 - bit];
        }
        return value;
    }

    //! Accessing the i-th element of the original bitvector
    value_type operator[](size_type i) const
    {
        assert(i < m_size);
        auto location = locate_block(i / k_block_size);
        return access_block(location.offset, location.descriptor, i % k_block_size);
    }

    //! Returns the size of the original bitvector
    size_type size() const
    {
        return m_size;
    }

    //! Serializes the data structure into the given ostream
    size_type serialize(std::ostream & out, structure_tree_node * v = nullptr, std::string name = "") const
    {
        structure_tree_node * child = structure_tree::add_child(v, name, util::class_name(*this));
        size_type written_bytes = 0;
        written_bytes += write_member(m_size, out, child, "size");
        written_bytes += m_trunk.serialize(out, child, "trunk");
        written_bytes += m_sblock_header.serialize(out, child, "sblock_header");
        written_bytes += m_hblock_header.serialize(out, child, "hblock_header");
        structure_tree::add_size(child, written_bytes);
        return written_bytes;
    }

    //! Loads the data structure from the given istream
    void load(std::istream & in)
    {
        read_member(m_size, in);
        m_trunk.load(in);
        m_sblock_header.load(in);
        m_hblock_header.load(in);
    }

    template <typename archive_t>
    void CEREAL_SAVE_FUNCTION_NAME(archive_t & ar) const
    {
        ar(CEREAL_NVP(m_size));
        ar(CEREAL_NVP(m_trunk));
        ar(CEREAL_NVP(m_sblock_header));
        ar(CEREAL_NVP(m_hblock_header));
    }

    template <typename archive_t>
    void CEREAL_LOAD_FUNCTION_NAME(archive_t & ar)
    {
        ar(CEREAL_NVP(m_size));
        ar(CEREAL_NVP(m_trunk));
        ar(CEREAL_NVP(m_sblock_header));
        ar(CEREAL_NVP(m_hblock_header));
    }

    iterator begin() const
    {
        return iterator(this, 0);
    }

    iterator end() const
    {
        return iterator(this, size());
    }

    bool operator==(hyb_vector const & v) const
    {
        return m_size == v.m_size && m_trunk == v.m_trunk && m_sblock_header == v.m_sblock_header
            && m_hblock_header == v.m_hblock_header;
    }

    bool operator!=(hyb_vector const & v) const
    {
        return !(*this == v);
    }
};


template <uint8_t t_bp>
struct rank_result
{
    typedef bit_vector::size_type size_type;
    static size_type adapt(size_type res, size_type)
    {
        return res;
    }
};

template <>
struct rank_result<0>
{
    typedef bit_vector::size_type size_type;
    static size_type adapt(size_type res, size_type i)
    {
        return i - res;
    }
};

//! Rank_support for the hyb_vector class
/*!
 * \tparam t_b      The bit pattern of size one. (so `0` or `1`)
 * \tparam k_sblock_rate  Superblock rate (number of blocks inside superblock)
 */
// TODO:
template <uint8_t t_b, uint32_t k_sblock_rate>
class rank_support_hyb
{
public:
    typedef hyb_vector<k_sblock_rate> bit_vector_type;
    typedef typename bit_vector_type::size_type size_type;
    enum
    {
        bit_pat = t_b
    };
    enum
    {
        bit_pat_len = (uint8_t)1
    };

private:
    bit_vector_type const * m_v;

    static uint32_t rank_plain(std::span<uint8_t const, bit_vector_type::k_block_bytes> data, uint32_t prefix_length)
    {
        return detail::rank_in_words(data, prefix_length);
    }

    static uint32_t rank_minority(std::span<uint8_t const> positions, bool minority_bit, uint32_t prefix_length)
    {
        uint32_t seen = std::ranges::count_if(positions, [&](uint8_t position) { return position < prefix_length; });
        return minority_bit ? seen : prefix_length - seen;
    }

    static uint32_t rank_runs(std::span<uint8_t const> ends, uint32_t ones, bool first_bit, uint32_t prefix_length)
    {
        uint32_t matched_before = 0;
        auto result = [&](uint32_t start, uint32_t end) {
            uint32_t prefix = matched_before + std::min(prefix_length, end) - std::min(prefix_length, start);
            return first_bit ? prefix : prefix_length - prefix;
        };
        uint32_t run = 0;
        uint32_t num_ends = std::min<uint32_t>(ends.size(), bit_vector_type::k_block_bytes);
        for (; run < num_ends; run += 2)
        {
            uint32_t start = run ? ends[run - 1] + 1 : 0;
            uint32_t end = ends[run] + 1;
            if (prefix_length <= end)
                return result(start, end);
            matched_before += end - start;
        }
        uint32_t population = first_bit ? ones : bit_vector_type::k_block_size - ones;
        uint32_t length = population - matched_before;
        uint32_t start = run == num_ends ? ends[num_ends - 1] + 1 : bit_vector_type::k_block_size - length;
        return result(start, start + length);
    }

#if __has_cpp_attribute(gnu::always_inline)
    [[gnu::always_inline]] // Keep block decoding inside the hot query path.
#endif
    uint32_t rank_block(size_type offset,
                        typename bit_vector_type::template block_descriptor<uint32_t> descriptor,
                        uint32_t prefix_length) const
    {
        auto [encoding_size, ones, special_bit] = descriptor;
        uint32_t zeros = bit_vector_type::k_block_size - ones;
        if (!encoding_size)
        {
            uint32_t first_run_length = special_bit ? ones : zeros;
            return special_bit ? std::min(prefix_length, first_run_length)
                               : prefix_length - std::min(prefix_length, first_run_length);
        }
        std::span<uint8_t const> data{&m_v->m_trunk[offset], encoding_size};
        if (encoding_size >= bit_vector_type::k_block_bytes)
            return rank_plain(data.first<bit_vector_type::k_block_bytes>(), prefix_length);
        if (encoding_size == std::min(ones, zeros))
            return rank_minority(data, special_bit, prefix_length);
        return rank_runs(data, ones, special_bit, prefix_length);
    }

public:
    //! Standard constructor
    explicit rank_support_hyb(bit_vector_type const * v = nullptr)
    {
        set_vector(v);
    }

    //! Answers rank queries
    size_type const rank(size_type i) const
    {
        assert(m_v != nullptr);
        assert(i <= m_v->size());

        // Handle easy case.
        if (i <= 0)
            return 0;

        auto last_position = i - 1;
        auto location = m_v->locate_block(last_position / bit_vector_type::k_block_size);
        auto local_rank = rank_block(
            location.offset, location.descriptor, last_position % bit_vector_type::k_block_size + 1);
        return rank_result<t_b>::adapt(location.ones_before + local_rank, i);
    }

    //! Shorthand for rank(i)
    size_type const operator()(size_type i) const
    {
        return rank(i);
    }

    //! Return the size of the original vector
    size_type const size() const
    {
        return m_v->size();
    }

    //! Set the supported vector
    void set_vector(bit_vector_type const * v = nullptr)
    {
        m_v = v;
    }

    //! Assignment operator
    rank_support_hyb & operator=(rank_support_hyb const & rs)
    {
        if (this != &rs)
        {
            set_vector(rs.m_v);
        }
        return *this;
    }

    //! Load the data structure from a stream and set the supported vector
    void load(std::istream &, bit_vector_type const * v = nullptr)
    {
        set_vector(v);
    }

    //! Serializes the data structure into a stream
    size_type serialize(std::ostream &, structure_tree_node * v = nullptr, std::string name = "") const
    {
        structure_tree_node * child = structure_tree::add_child(v, name, util::class_name(*this));
        structure_tree::add_size(child, 0);
        return 0;
    }

    template <typename archive_t>
    void CEREAL_SAVE_FUNCTION_NAME(archive_t &) const
    {}

    template <typename archive_t>
    void CEREAL_LOAD_FUNCTION_NAME(archive_t &)
    {}

    bool operator==(rank_support_hyb const & other) const noexcept
    {
        return *m_v == *other.m_v;
    }

    bool operator!=(rank_support_hyb const & other) const noexcept
    {
        return !(*this == other);
    }
};

//! Select support for the hyb_vector class
/*!
 * \tparam t_b            The bit pattern of size one. (so `0` or `1`)
 * \tparam k_sblock_rate  Superblock rate (number of blocks inside superblock)
 * Select samples occurrence positions to narrow the superblock search, then decodes the selected block.
 * Samples occupy about size()/128 bits per support and are rebuilt when attaching or loading.
 * They are not serialized; size_in_bytes therefore excludes this derived memory.
 */
template <uint8_t t_b, uint32_t k_sblock_rate>
class select_support_hyb
{
public:
    typedef hyb_vector<k_sblock_rate> bit_vector_type;
    typedef typename bit_vector_type::size_type size_type;
    enum
    {
        bit_pat = t_b
    };
    enum
    {
        bit_pat_len = (uint8_t)1
    };

private:
    bit_vector_type const * m_v;

    // Chiu and Kempa, SEA 2026, Section 4: sample occurrences to bound the search.
    // The directory is derived state, rebuilt on attachment rather than serialized.
    static constexpr size_type sample_ratio = 128;
    size_type m_sample_interval = 1;
    int_vector<64> m_samples;

    auto populations() const
    {
        auto population = [this](size_type sblock)
        {
            auto local = m_v->superblock_header(sblock).relative_ones;
            auto hblock = sblock * k_sblock_rate / bit_vector_type::k_hblock_rate;
            return rank_result<t_b>::adapt(m_v->hyperblock_header(hblock).ones_before + local,
                                           sblock * bit_vector_type::k_sblock_size);
        };
        return random_access_container(population, (size() - 1) / bit_vector_type::k_sblock_size + 1);
    }

    void initialize_samples()
    {
        if (!m_v || !size())
        {
            m_sample_interval = 1;
            m_samples = int_vector<64>();
            return;
        }
        auto last = m_v->locate_block((size() - 1) / bit_vector_type::k_block_size);
        size_type total = rank_result<t_b>::adapt(last.ones_before + last.descriptor.ones, size());
        size_type budget = std::max<size_type>(size() / sample_ratio / 64, 2) - 1;
        m_sample_interval = std::max<size_type>((total + budget - 1) / budget, 1);
        size_type count = (total + m_sample_interval - 1) / m_sample_interval;
        m_samples = int_vector<64>(count + 1);
        auto prefixes = populations();
        size_type sblock = 0;
        for (size_type sample = 0; sample < count; ++sample)
        {
            size_type target = sample * m_sample_interval + 1;
            while (sblock + 1 < prefixes.size() && prefixes[sblock + 1] < target)
                ++sblock;
            m_samples[sample] = sblock;
        }
        m_samples[count] = prefixes.size() - 1;
    }

    using descriptor_type = typename bit_vector_type::template block_descriptor<uint32_t>;

    static uint32_t select_plain(std::span<uint8_t const, bit_vector_type::k_block_bytes> data, uint8_t select_index)
    {
        return detail::select_in_words<t_b>(data, select_index + 1);
    }

    static uint32_t select_minority(std::span<uint8_t const> positions, bool minority_bit, uint8_t select_index)
    {
        if (t_b == minority_bit)
            return positions[select_index];
        auto indices = std::views::iota(uint8_t{0}, uint8_t(positions.size()));
        // Skip the j preceding minority positions.
        uint32_t seen = std::ranges::count_if(indices, [&](uint8_t j) { return positions[j] - j <= select_index; });
        return select_index + seen;
    }

    static uint32_t select_runs(std::span<uint8_t const> ends, uint32_t ones, bool first_bit, uint8_t select_index)
    {
        uint32_t matched_before = 0;
        uint32_t run = first_bit != t_b;
        uint32_t num_ends = std::min<uint32_t>(ends.size(), bit_vector_type::k_block_bytes);
        for (; run < num_ends; run += 2)
        {
            uint32_t start = run ? ends[run - 1] + 1 : 0;
            uint32_t end = ends[run] + 1;
            if (select_index < matched_before + end - start)
                return start + select_index - matched_before;
            matched_before += end - start;
        }
        uint32_t population = t_b ? ones : bit_vector_type::k_block_size - ones;
        uint32_t length = population - matched_before;
        uint32_t start = run == num_ends ? ends[num_ends - 1] + 1 : bit_vector_type::k_block_size - length;
        return start + select_index - matched_before;
    }

#if __has_cpp_attribute(gnu::always_inline)
    [[gnu::always_inline]] // Keep block decoding inside the hot query path.
#endif
    uint32_t select_block(size_type offset, descriptor_type descriptor, uint32_t select_index) const
    {
        auto [encoding_size, ones, special_bit] = descriptor;
        uint32_t zeros = bit_vector_type::k_block_size - ones;
        if (!encoding_size)
        {
            uint32_t first_run_length = special_bit ? ones : zeros;
            return (t_b == special_bit ? 0 : first_run_length) + select_index;
        }
        std::span<uint8_t const> data{&m_v->m_trunk[offset], encoding_size};
        if (encoding_size >= bit_vector_type::k_block_bytes)
            return select_plain(data.first<bit_vector_type::k_block_bytes>(), select_index);
        if (encoding_size == std::min(ones, zeros))
            return select_minority(data, special_bit, select_index);
        return select_runs(data, ones, special_bit, select_index);
    }


public:
    //! Standard constructor
    explicit select_support_hyb(bit_vector_type const * v = nullptr)
    {
        set_vector(v);
    }

    //! Search superblock populations, then the selected block.
    size_type select(size_type i) const
    {
        assert(i > 0);
        if (!size())
            return 0;
        // The final block is zero-padded, so its descriptor gives its full population.
        auto last = m_v->locate_block((size() - 1) / bit_vector_type::k_block_size);
        auto total_ones = last.ones_before + last.descriptor.ones;
        if (i > rank_result<t_b>::adapt(total_ones, size()))
            return size();

        auto prefixes = populations();
        size_type sample = (i - 1) / m_sample_interval;
        auto first = std::ranges::lower_bound(prefixes.begin() + m_samples[sample] + 1,
                                              prefixes.begin() + m_samples[sample + 1] + 1, i);
        // Prefix zero is below i, so the target superblock precedes the lower bound.
        size_type begin = first - prefixes.begin() - 1;

        i -= prefixes[begin];
        size_type base = begin * bit_vector_type::k_sblock_size;
        auto [relative_offset, relative_ones, uniform, descriptors] = m_v->superblock_header(begin);
        if (uniform)
            return base + i - 1;

        auto hblock = begin * k_sblock_rate / bit_vector_type::k_hblock_rate;
        size_type offset = m_v->hyperblock_header(hblock).payload_offset + relative_offset;
        descriptor_type descriptor;
        uint32_t count;
        for (;; descriptors = descriptors.subspan(2), base += bit_vector_type::k_block_size)
        {
            descriptor = bit_vector_type::template decode_block<uint32_t>(detail::read_unaligned<uint16_t>(descriptors));
            count = rank_result<t_b>::adapt(descriptor.ones, bit_vector_type::k_block_size);
            if (i <= count)
                break;
            i -= count;
            offset += descriptor.payload_bytes;
        }

        return base + select_block(offset, descriptor, i - 1);
    }

    //! Shorthand for select(i)
    size_type const operator()(size_type i) const
    {
        return select(i);
    }

    //! Return the size of the original vector
    size_type const size() const
    {
        return m_v->size();
    }

    //! Set the supported vector
    void set_vector(bit_vector_type const * v = nullptr)
    {
        m_v = v;
        initialize_samples();
    }

    //! Load the data structure from a stream and set the supported vector
    void load(std::istream &, bit_vector_type const * v = nullptr)
    {
        set_vector(v);
    }

    //! Serializes the data structure into a stream
    size_type serialize(std::ostream &, structure_tree_node * v = nullptr, std::string name = "") const
    {
        structure_tree_node * child = structure_tree::add_child(v, name, util::class_name(*this));
        structure_tree::add_size(child, 0);
        return 0;
    }

    template <typename archive_t>
    void CEREAL_SAVE_FUNCTION_NAME(archive_t &) const
    {}

    template <typename archive_t>
    void CEREAL_LOAD_FUNCTION_NAME(archive_t &)
    {
        initialize_samples();
    }

    bool operator==(select_support_hyb const & other) const noexcept
    {
        return *m_v == *other.m_v;
    }

    bool operator!=(select_support_hyb const & other) const noexcept
    {
        return !(*this == other);
    }
};

} // end namespace sdsl

#endif // INCLUDED_SDSL_HYB_VECTOR
