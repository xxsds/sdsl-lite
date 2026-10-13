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
#include <cstring>
#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <string>

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
    static uint32_t const k_block_bytes;
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
                m_hblock_header[2 * hblock_id] = trunk_ptr;
                m_hblock_header[2 * hblock_id + 1] = tot_rank;
            }

            // Update sblock header.
            if (!(block_id % k_sblock_rate))
            {
                uint32_t * ptr = (uint32_t *)(((uint8_t *)m_sblock_header.data()) + k_sblock_header_size * sblock_id);
                *ptr++ = trunk_ptr - m_hblock_header[2 * hblock_id];
                *ptr = tot_rank - m_hblock_header[2 * hblock_id + 1];

                // If the sblock is uniform, flip the bit.
                if (sblock_id && (!sblock_ones || sblock_ones == k_sblock_size))
                {
                    ptr = (uint32_t *)(((uint8_t *)m_sblock_header.data()) + k_sblock_header_size * (sblock_id - 1));
                    *ptr |= 0x80000000;
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
            uint16_t * header_ptr16 =
                (uint16_t *)(((uint8_t *)m_sblock_header.data()) + sblock_id * k_sblock_header_size + 8
                             + (block_id % k_sblock_rate) * 2);

            (*header_ptr16) = ones;
            if (ones == k_block_size)
                (*header_ptr16) |= 0x200;

            if (0 < ones && ones < k_block_size)
            { // non uniform block
                uint32_t minority_enc_size = std::min(ones, zeros);
                uint32_t runs_enc_size = (uint32_t)std::max(0, (int32_t)runs - 2);
                uint32_t best_enc_size = std::min(minority_enc_size, runs_enc_size);

                if (k_block_bytes <= best_enc_size)
                {
                    // Use plain encoding.
                    (*header_ptr16) |= (k_block_bytes << 10);

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
                        (*header_ptr16) |= (runs_enc_size << 10);
                        (*header_ptr16) |= (bv[block_beg] << 9);

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
                        (*header_ptr16) |= (minority_enc_size << 10);
                        if (ones < zeros)
                            (*header_ptr16) |= 0x200;
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

            // Update global rank.
            tot_rank += ones;
            sblock_ones += ones;
            bv_ptr += k_block_size / 64;
        }
    }

private:
    template <typename T>
    static T read_unaligned(uint8_t const * data)
    {
        T value;
        std::memcpy(&value, data, sizeof(value));
        return value;
    }

    // Access uses position + 1, rank a prefix length, and select an occurrence rank.
    // Access/rank report bits/ones; t_b chooses the bit only for select.
    enum class block_query
    {
        access,
        rank,
        select
    };

    template <block_query t_query, uint8_t t_b>
    static uint32_t query_plain(uint8_t const * data, uint32_t i)
    {
        if constexpr (t_query == block_query::access)
            return (read_unaligned<uint64_t>(data + 8 * ((i - 1) / 64)) >> ((i - 1) % 64)) & 1;
        uint32_t accumulated = 0;
        for (uint32_t word = 0; word < k_block_bytes / 8; ++word)
        {
            uint64_t value = read_unaligned<uint64_t>(data + 8 * word);
            if constexpr (t_query == block_query::select)
            {
                if constexpr (t_b == 0)
                    value = ~value;
                auto count = bits::cnt(value);
                if (i <= count)
                    return 64 * word + bits::sel(value, i);
                i -= count;
            }
            else
            {
                if (i <= 64)
                    return accumulated + bits::cnt(value << (64 - i));
                accumulated += bits::cnt(value);
                i -= 64;
            }
        }
        assert(false && "Invalid hybrid block population");
        return 0;
    }

    template <block_query t_query, uint8_t t_b>
    static uint32_t query_minority(uint8_t const * positions, uint32_t num_positions, uint32_t minority_bit, uint32_t i)
    {
        // Minority positions are explicit; majority select skips these holes.
        if constexpr (t_query == block_query::select)
            if (t_b == minority_bit)
                return positions[i - 1];
        uint32_t seen = 0, limit = i;
        do
        {
            if (positions[seen] >= limit)
                break;
            ++seen;
            if constexpr (t_query == block_query::select)
                ++limit;
        }
        while (seen < num_positions);
        if constexpr (t_query == block_query::select)
            return limit - 1;
        else if constexpr (t_query == block_query::access)
            return (seen && positions[seen - 1] == i - 1) ? minority_bit : !minority_bit;
        else
            return minority_bit ? seen : i - seen;
    }

    template <block_query t_query, uint8_t t_b>
    static uint32_t query_runs(uint8_t const * ends, uint32_t num_ends, uint32_t ones, uint32_t first_bit, uint32_t i)
    {
        // Access/rank follow the first bit's runs; select follows t_b's runs.
        uint32_t matching_bit = t_query == block_query::select ? t_b : first_bit;
        uint32_t population = matching_bit ? ones : k_block_size - ones;
        uint32_t matched_before = 0; // Matching-bit population before the current run.
        uint32_t start = 0, end = 0;
        uint32_t run = first_bit != matching_bit;
        // Run encoding stores 1..31 ends; keep this bound visible to the optimizer.
        num_ends = std::min(num_ends, k_block_bytes);
        for (; run < num_ends; run += 2)
        {
            start = run ? ends[run - 1] + 1 : 0;
            end = ends[run] + 1;
            if (i <= (t_query == block_query::select ? matched_before + end - start : end))
            {
                if constexpr (t_query == block_query::access)
                    return i > start ? matching_bit : !matching_bit;
                break;
            }
            matched_before += end - start;
        }
        // The final two run ends are implicit in the total population.
        if (run >= num_ends)
        {
            uint32_t length = population - matched_before;
            start = run == num_ends ? ends[num_ends - 1] + 1 : k_block_size - length;
            end = start + length;
        }

        if constexpr (t_query == block_query::select)
            return start + i - matched_before - 1;
        if constexpr (t_query == block_query::access)
            return start < i && i <= end ? matching_bit : !matching_bit;
        uint32_t prefix = matched_before + std::clamp(i, start, end) - start;
        return matching_bit ? prefix : i - prefix;
    }

    template <block_query t_query, uint8_t t_b = 1>
#if __has_cpp_attribute(gnu::always_inline)
    [[gnu::always_inline]] // Keep block decoding inside these hot query paths.
#endif
    uint32_t query_block(size_type offset, uint16_t descriptor, uint32_t i) const
    {
        uint32_t encoding_size = descriptor >> 10;
        uint32_t ones = descriptor & 0x1ff;
        uint32_t zeros = k_block_size - ones;
        uint32_t special_bit = (descriptor >> 9) & 1;

        // At most two runs; their lengths follow from the population.
        if (!encoding_size)
        {
            uint32_t first_run_length = special_bit ? ones : zeros;
            if constexpr (t_query == block_query::select)
                return (t_b == special_bit ? 0 : first_run_length) + i - 1;
            else if constexpr (t_query == block_query::access)
                return (i > first_run_length) ^ special_bit;
            else
                return special_bit ? std::min(i, first_run_length) : i - std::min(i, first_run_length);
        }

        auto data = m_trunk.begin() + offset;
        if (encoding_size >= k_block_bytes)
            return query_plain<t_query, t_b>(data, i);
        if (encoding_size == std::min(ones, zeros))
            return query_minority<t_query, t_b>(data, encoding_size, special_bit, i);
        return query_runs<t_query, t_b>(data, encoding_size, ones, special_bit, i);
    }

    struct block_location
    {
        size_type offset;
        size_type ones_before;
        uint16_t descriptor;
    };

#if __has_cpp_attribute(gnu::always_inline)
    [[gnu::always_inline]] // Specialize the locator for the fields its caller uses.
#endif
    block_location locate_block(size_type block_id) const
    {
        auto sblock = block_id / k_sblock_rate;
        auto hblock = block_id / k_hblock_rate;
        auto header = m_sblock_header.begin() + sblock * k_sblock_header_size;
        auto packed = read_unaligned<uint32_t>(header);
        auto local_rank = read_unaligned<uint32_t>(header + 4);
        block_location result{m_hblock_header[2 * hblock] + (packed & 0x3fffffff),
                              m_hblock_header[2 * hblock + 1] + local_rank, 0};
        header += 8;
        auto preceding = block_id % k_sblock_rate;
        if (packed & 0x80000000) // Every block in this superblock has the same bit.
        {
            result.descriptor = read_unaligned<uint16_t>(header);
            result.ones_before += (result.descriptor & 0x1ff) * preceding;
            return result;
        }
#if defined(__GNUC__) || defined(__clang__)
        if constexpr (k_sblock_rate == 16)
        {
            // Every superblock allocates all sixteen descriptors, including the final one.
            using lanes_type = uint16_t __attribute__((vector_size(16)));
            auto lo = read_unaligned<lanes_type>(header);
            auto hi = read_unaligned<lanes_type>(header + sizeof(lo));
            lanes_type const indices = {0, 1, 2, 3, 4, 5, 6, 7};
            lo &= static_cast<lanes_type>(indices < static_cast<uint16_t>(preceding));
            hi &= static_cast<lanes_type>(indices + 8 < static_cast<uint16_t>(preceding));
            auto sizes = (lo >> 10) + (hi >> 10);
            auto populations = (lo & 0x1ff) + (hi & 0x1ff);
            uint32_t bytes = 0, ones = 0;
            for (uint32_t j = 0; j < 8; ++j)
            {
                bytes += sizes[j];
                ones += populations[j];
            }
            result.offset += bytes;
            result.ones_before += ones;
        }
        else
#endif
        {
            for (size_type j = 0; j < preceding; ++j)
            {
                auto descriptor = read_unaligned<uint16_t>(header + 2 * j);
                result.offset += descriptor >> 10;
                result.ones_before += descriptor & 0x1ff;
            }
        }
        result.descriptor = read_unaligned<uint16_t>(header + 2 * preceding);
        return result;
    }

    //! Given i returns bv[i - 1].
    value_type access0(size_type i) const
    {
        assert(i > 0);
        assert(i <= m_size);
        auto location = locate_block((i - 1) / k_block_size);
        return query_block<block_query::access>(location.offset, location.descriptor, (i - 1) % k_block_size + 1);
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
uint32_t const hyb_vector<k_sblock_rate>::k_block_bytes = 32;
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

        auto location = m_v->locate_block((i - 1) / bit_vector_type::k_block_size);
        auto local_rank = m_v->template query_block<bit_vector_type::block_query::rank>(
            location.offset, location.descriptor, (i - 1) % bit_vector_type::k_block_size + 1);
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
 * Select queries narrow the search using superblock boundaries.
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

    //! Search superblock populations, then the selected block.
    size_type select(size_type i) const
    {
        assert(i > 0);
        if (!size())
            return 0;
        // The final block is zero-padded, so its descriptor gives its full population.
        auto last = m_v->locate_block((size() - 1) / bit_vector_type::k_block_size);
        auto total_ones = last.ones_before + (last.descriptor & 0x1ff);
        if (i > rank_result<t_b>::adapt(total_ones, size()))
            return size();

        // Superblock headers contain prefix populations relative to their hyperblock.
        // Read packed metadata with memcpy because the headers need not be aligned.
        auto header = [&](size_type sblock)
        {
            return m_v->m_sblock_header.begin() + sblock * bit_vector_type::k_sblock_header_size;
        };
        auto population = [&](size_type sblock)
        {
            auto local = m_v->template read_unaligned<uint32_t>(header(sblock) + 4);
            auto hblock = sblock * k_sblock_rate / bit_vector_type::k_hblock_rate;
            return rank_result<t_b>::adapt(m_v->m_hblock_header[2 * hblock + 1] + local,
                                           sblock * bit_vector_type::k_sblock_size);
        };
        auto prefixes = random_access_container(population, (size() - 1) / bit_vector_type::k_sblock_size + 1);
        auto first = std::ranges::lower_bound(prefixes.begin() + 1, prefixes.end(), i);
        // Prefix zero is below i, so the target superblock precedes the lower bound.
        size_type begin = first - prefixes.begin() - 1;

        i -= population(begin);
        size_type base = begin * bit_vector_type::k_sblock_size;
        auto header_ptr = header(begin);
        auto packed = m_v->template read_unaligned<uint32_t>(header_ptr);
        if (packed & 0x80000000) // Uniform superblock.
            return base + i - 1;

        auto hblock = begin * k_sblock_rate / bit_vector_type::k_hblock_rate;
        size_type offset = m_v->m_hblock_header[2 * hblock] + (packed & 0x3fffffff);
        header_ptr += 8;
        uint16_t descriptor;
        uint32_t count;
        for (;; header_ptr += 2, base += bit_vector_type::k_block_size)
        {
            descriptor = m_v->template read_unaligned<uint16_t>(header_ptr);
            count = rank_result<t_b>::adapt(descriptor & 0x1ff, bit_vector_type::k_block_size);
            if (i <= count)
                break;
            i -= count;
            offset += descriptor >> 10;
        }

        return base + m_v->template query_block<bit_vector_type::block_query::select, t_b>(offset, descriptor, i);
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
