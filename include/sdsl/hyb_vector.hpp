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
 *     - TODO: added a naive implementation of select
 */
#ifndef INCLUDED_SDSL_HYB_VECTOR
#define INCLUDED_SDSL_HYB_VECTOR

#include <algorithm>
#include <assert.h>
#include <cstdlib>
#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <span>
#include <type_traits>

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
    static uint32_t const k_block_size;
    static constexpr uint32_t k_block_bytes = 32;
    static uint32_t const k_sblock_header_size;
    static uint32_t const k_sblock_size;
    static uint32_t const k_hblock_rate;

    size_type m_size = 0;           // original bitvector size
    int_vector<8> m_trunk;          // body of encoded blocks
    int_vector<8> m_sblock_header;  // sblock headers
    int_vector<64> m_hblock_header; // hblock headers

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
                std::span<uint8_t> header{&m_sblock_header[sblock_id * k_sblock_header_size],
                                         k_sblock_header_size};
                auto [payload_offset, ones_before] = hyperblock_header(hblock_id);
                detail::write_unaligned<uint32_t>(header, trunk_ptr - payload_offset);
                detail::write_unaligned<uint32_t>(header, tot_rank - ones_before, 1);

                // If the sblock is uniform, flip the bit.
                if (sblock_id && (!sblock_ones || sblock_ones == k_sblock_size))
                {
                    std::span<uint8_t> previous{&m_sblock_header[(sblock_id - 1) * k_sblock_header_size],
                                               k_sblock_header_size};
                    auto packed = detail::read_unaligned<uint32_t>(previous);
                    detail::write_unaligned(previous, packed | (uint32_t{1} << 31));
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
                descriptor |= 0x200;

            if (0 < ones && ones < k_block_size)
            { // non uniform block
                uint32_t minority_enc_size = std::min(ones, zeros);
                uint32_t runs_enc_size = (uint32_t)std::max(0, (int32_t)runs - 2);
                uint32_t best_enc_size = std::min(minority_enc_size, runs_enc_size);

                if (k_block_bytes <= best_enc_size)
                {
                    // Use plain encoding.
                    descriptor |= (k_block_bytes << 10);

                    // Copy original 256 bits from bv into trunk.
                    if (block_end <= m_size)
                    {
                        for (uint8_t i = 0; i < 4; ++i)
                        {
                            *((uint64_t *)(((uint8_t *)m_trunk.data()) + trunk_ptr)) = *(bv_ptr + i);
                            trunk_ptr += 8;
                        }
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
                            *((uint64_t *)(((uint8_t *)m_trunk.data()) + trunk_ptr)) = w;
                            trunk_ptr += 8;
                        }
                    }
                }
                else
                {
                    if (runs_enc_size < minority_enc_size)
                    {

                        // Use runs encoding.
                        descriptor |= (runs_enc_size << 10);
                        descriptor |= (bv[block_beg] << 9);

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
                        descriptor |= (minority_enc_size << 10);
                        if (ones < zeros)
                            descriptor |= 0x200;
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

            std::span<uint8_t> descriptors{&m_sblock_header[sblock_id * k_sblock_header_size + 8],
                                          2 * k_sblock_rate};
            detail::write_unaligned(descriptors, descriptor, block_id % k_sblock_rate);

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
        std::span<uint8_t const> bytes{m_sblock_header.begin() + sblock * k_sblock_header_size,
                                      k_sblock_header_size};
        auto packed = detail::read_unaligned<uint32_t>(bytes);
        return {static_cast<uint32_t>(packed & bits::lo_set[30]),
                detail::read_unaligned<uint32_t>(bytes, 1),
                (packed & (uint32_t{1} << 31)) != 0, bytes.subspan(8)};
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
        auto data = m_hblock_header.data() + 2 * hblock;
        return {data[0], data[1]};
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
        return {packed >> 10, packed & 0x1ff,
                static_cast<typename block_descriptor<T>::bit_type>((packed >> 9) & 1)};
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
        constexpr uint32_t lanes = k_sblock_rate < 8 ? 1 : 8;
        // Use native-width scalar sums; widen SIMD lanes only when their sums could overflow.
        constexpr uint64_t max_lane_sum = ((uint64_t{k_sblock_rate} + lanes - 1) / lanes) * 256;
        using lane_type = std::conditional_t<(lanes != 1 && max_lane_sum <= UINT16_MAX), uint16_t, uint64_t>;
        using packed_type [[gnu::vector_size(lanes * sizeof(uint16_t))]] = uint16_t;
        using lanes_type [[gnu::vector_size(lanes * sizeof(lane_type))]] = lane_type;
        lanes_type indices{};
        for (uint32_t lane = 0; lane < lanes; ++lane)
            indices[lane] = lane;
        lanes_type sizes{}, populations{};
        auto end = lanes == 1 ? preceding : k_sblock_rate;
        for (size_type j = 0; j < end; j += lanes)
        {
            packed_type packed{};
            std::memcpy(&packed, descriptors.data() + 2 * j,
                        2 * std::min<size_type>(lanes, k_sblock_rate - j));
            auto values = __builtin_convertvector(packed, lanes_type);
            if constexpr (lanes > 1)
                values &= static_cast<lanes_type>(indices + static_cast<lane_type>(j)
                                                  < static_cast<lane_type>(preceding));
            auto decoded = decode_block(values);
            sizes += decoded.payload_bytes;
            populations += decoded.ones;
        }
        using total_type = decltype(lane_type{} + lane_type{});
        total_type bytes = 0, ones = 0;
        for (uint32_t lane = 0; lane < lanes; ++lane)
        {
            bytes += sizes[lane];
            ones += populations[lane];
        }
        result.offset += bytes;
        result.ones_before += ones;
        result.descriptor = decode_block<uint32_t>(detail::read_unaligned<uint16_t>(descriptors, preceding));
        return result;
    }


    //! Given i returns bv[i - 1].
    value_type access0(size_type i) const
    {
        assert(i > 0);
        assert(i <= m_size);

        size_type block_id = (i - 1) / k_block_size;
        auto location = locate_block(block_id);
        uint32_t local_i = i - block_id * k_block_size;
        auto [encoding_size, ones, special_bit] = location.descriptor;
        uint32_t zeros = k_block_size - ones;

        // Number of runs <= 2.
        if (!encoding_size)
        {
            uint32_t first_run_length = special_bit * ones + (1 - special_bit) * zeros;
            uint8_t inside_second_run = (first_run_length < local_i);
            return (inside_second_run ^ special_bit);
        }

        uint8_t const * trunk_p = m_trunk.begin() + location.offset;

        // Number of runs > 2.
        if (encoding_size < k_block_bytes)
        {
            if (std::min(ones, zeros) == encoding_size)
            {
                // Minority encoding.
                uint32_t tot = 0;
                while (tot < encoding_size && *trunk_p < local_i)
                {
                    ++trunk_p;
                    ++tot;
                }
                uint8_t last_was_majority = ((!tot) || (*(trunk_p - 1) != local_i - 1));
                return (last_was_majority ^ special_bit);
            }

            // Runs encoding.
            if (special_bit)
            {
                uint32_t j = 0;
                uint32_t acc = 0;

                int32_t last = -1;
                while (j + 1 < encoding_size && *(trunk_p + 1) < local_i)
                {
                    acc += *trunk_p - last;
                    ++trunk_p;
                    last = *trunk_p;
                    ++trunk_p;
                    j += 2;
                }

                uint8_t access_i = 0;
                if (j + 1 >= encoding_size)
                {
                    if (j < encoding_size)
                    { // j == encoding_size - 1
                        if (local_i <= (uint32_t)(*trunk_p) + 1)
                            access_i = (((int32_t)local_i - last - 1) > 0);
                        else
                        {
                            acc += (int32_t)(*trunk_p) - last;
                            if (ones - acc <= k_block_size - local_i)
                                access_i = 0;
                            else
                                access_i = 1;
                        }
                    }
                    else
                    { // j == encoding_size
                        if ((int32_t)(ones - acc) < (int32_t)local_i - last - 1)
                            access_i = 0;
                        else
                            access_i = (((int32_t)local_i - last - 1) > 0);
                    }
                }
                else
                {
                    if ((*trunk_p) < local_i - 1)
                        access_i = 0;
                    else
                        access_i = (((int32_t)local_i - last - 1) > 0);
                }

                return access_i;
            }
            else
            {
                uint32_t j = 0;
                uint32_t acc = 0;
                int32_t last = -1;

                while (j + 1 < encoding_size && *(trunk_p + 1) < local_i)
                {
                    acc += *trunk_p - last;
                    ++trunk_p;
                    last = *trunk_p;
                    ++trunk_p;
                    j += 2;
                }

                uint8_t access_i = 0;
                if (j + 1 >= encoding_size)
                {
                    if (j < encoding_size)
                    {
                        if (local_i <= (uint32_t)(*trunk_p) + 1)
                            access_i = (((int32_t)local_i - last - 1) == 0);
                        else
                        {
                            acc += (*trunk_p) - last;
                            if (zeros - acc <= k_block_size - local_i)
                                access_i = 1;
                            else
                                access_i = 0;
                        }
                    }
                    else
                    {
                        if ((int32_t)(zeros - acc) < (int32_t)local_i - last - 1)
                            access_i = 1;
                        else
                            access_i = ((local_i - last - 1) == 0);
                    }
                }
                else
                {
                    if ((*trunk_p) < local_i - 1)
                        access_i = 1;
                    else
                        access_i = (((int32_t)local_i - last - 1) == 0);
                }

                return access_i;
            }
        }
        else
        {
            // plain encoding.
            uint64_t * trunk_ptr64 = (uint64_t *)(((uint8_t *)m_trunk.data()) + location.offset);
            uint32_t bit;
            for (bit = 0; bit + 64 <= local_i; bit += 64)
                trunk_ptr64++;

            uint8_t access_i = 0;
            if (bit != local_i)
                access_i = (((*trunk_ptr64) >> (local_i - bit - 1)) & 1);
            else
                access_i = (((*(trunk_ptr64 - 1)) >> 63) & 1);
            return access_i;
        }
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
    uint64_t get_int(size_type idx, uint8_t const len = 64) const
    {
        uint64_t res = 0;
        for (size_t i = 0; i < len; ++i)
        {
            res <<= 1;
            res |= (*this)[idx + len - 1 - i];
        }
        return res;
    }

    //! Accessing the i-th element of the original bitvector
    value_type operator[](size_type i) const
    {
        return access0(i + 1);
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

template <uint32_t k_sblock_rate>
uint32_t const hyb_vector<k_sblock_rate>::k_block_size = 256;
template <uint32_t k_sblock_rate>
uint32_t const hyb_vector<k_sblock_rate>::k_sblock_header_size = 8 + 2 * k_sblock_rate;
template <uint32_t k_sblock_rate>
uint32_t const hyb_vector<k_sblock_rate>::k_sblock_size = 256 * k_sblock_rate;
template <uint32_t k_sblock_rate>
uint32_t const hyb_vector<k_sblock_rate>::k_hblock_rate = (1U << 31) / 256;

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

        size_type block_id = (i - 1) / bit_vector_type::k_block_size;
        auto location = m_v->locate_block(block_id);
        uint32_t local_i = i - block_id * bit_vector_type::k_block_size;
        size_type block_rank = location.ones_before;
        auto [encoding_size, ones, special_bit] = location.descriptor;
        uint32_t zeros = bit_vector_type::k_block_size - ones;

        // Number of runs <= 2.
        if (!encoding_size)
        {
            uint32_t first_run_length = special_bit * ones + (1 - special_bit) * zeros;
            uint32_t local_rank = std::min(local_i, first_run_length);

            return rank_result<t_b>::adapt(
                block_rank + (special_bit * local_rank + (1 - special_bit) * (local_i - local_rank)), i);
        }

        uint8_t const * trunk_p = m_v->m_trunk.begin() + location.offset;

        // Number of runs > 2.
        if (encoding_size < bit_vector_type::k_block_bytes)
        {
            if (std::min(ones, zeros) == encoding_size)
            {
                // Minority encoding.
                uint32_t tot = 0;
                while (tot < encoding_size && (*trunk_p++) < local_i)
                    ++tot;

                return rank_result<t_b>::adapt(block_rank + special_bit * tot + (1 - special_bit) * (local_i - tot), i);
            }

            // Runs encoding.
            if (special_bit)
            {
                uint32_t j = 0;
                uint32_t acc = 0;

                int32_t last = -1;
                while (j + 1 < encoding_size && *(trunk_p + 1) < local_i)
                {
                    acc += *trunk_p - last;
                    ++trunk_p;
                    last = *trunk_p;
                    ++trunk_p;
                    j += 2;
                }

                if (j + 1 >= encoding_size)
                {
                    if (j < encoding_size)
                    {
                        if (*trunk_p >= local_i)
                            acc += local_i - last - 1;
                        else
                        {
                            acc += (*trunk_p) - last;
                            acc += (ones - acc) - std::min(ones - acc, bit_vector_type::k_block_size - local_i);
                        }
                    }
                    else
                        acc += std::min(ones - acc, local_i - last - 1);
                }
                else
                    acc += std::min((int32_t)(*trunk_p), (int32_t)local_i - 1) - last;

                return rank_result<t_b>::adapt(block_rank + acc, i);
            }
            else
            {
                uint32_t j = 0;
                uint32_t acc = 0;
                int32_t last = -1;

                while (j + 1 < encoding_size && *(trunk_p + 1) < local_i)
                {
                    acc += *trunk_p - last;
                    ++trunk_p;
                    last = *trunk_p;
                    ++trunk_p;
                    j += 2;
                }

                if (j + 1 >= encoding_size)
                {
                    if (j < encoding_size)
                    {
                        if (*trunk_p >= local_i)
                            acc += local_i - last - 1;
                        else
                        {
                            acc += (*trunk_p) - last;
                            acc += (zeros - acc) - std::min(zeros - acc, bit_vector_type::k_block_size - local_i);
                        }
                    }
                    else
                        acc += std::min(zeros - acc, local_i - last - 1);
                }
                else
                    acc += std::min((int32_t)(*trunk_p), (int32_t)local_i - 1) - last;

                return rank_result<t_b>::adapt(block_rank + (local_i - acc), i);
            }
        }
        else
        {
            // plain encoding.
            uint64_t * trunk_ptr64 = (uint64_t *)(((uint8_t *)m_v->m_trunk.data()) + location.offset);
            uint32_t bit;
            for (bit = 0; bit + 64 <= local_i; bit += 64)
                block_rank += bits::cnt(*trunk_ptr64++);
            if (bit != local_i)
                block_rank += bits::cnt((*trunk_ptr64) & (((uint64_t)1 << (local_i - bit)) - 1));

            return rank_result<t_b>::adapt(block_rank, i);
        }
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
 * Select queries use a logarithmic number of rank queries.
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

public:
    //! Standard constructor
    explicit select_support_hyb(bit_vector_type const * v = nullptr)
    {
        set_vector(v);
    }

    //! Answers select queries using binary search on rank.
    size_type select(size_type i) const
    {
        assert(i > 0);
        rank_support_hyb<t_b, k_sblock_rate> const rank(m_v);
        if (i > rank(size()))
            return size();

        size_type begin = 0, end = size();
        while (end - begin > 1)
        {
            size_type const mid = begin + (end - begin) / 2;
            if (rank(mid) < i)
                begin = mid;
            else
                end = mid;
        }
        return begin;
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
    }

    //! Assignment operator
    select_support_hyb & operator=(select_support_hyb const & rs)
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
