// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
/*!\file rank_support_int_v.hpp
 * \brief rank_support_int_v.hpp contains rank_support_int_v.
 * \author Christopher Pockrandt
 * \author René Rahn
 */
#ifndef INCLUDED_SDSL_RANK_SUPPORT_INT_V
#define INCLUDED_SDSL_RANK_SUPPORT_INT_V

#include <algorithm>
#include <array>
#include <assert.h>
#include <iosfwd>
#include <iterator>
#include <stddef.h>
#include <stdint.h>
#include <string>
#include <type_traits>
#include <vector>

#include <sdsl/bits.hpp>
#include <sdsl/cereal.hpp>
#include <sdsl/int_vector.hpp>
#include <sdsl/io.hpp>
#include <sdsl/rank_support_int.hpp>
#include <sdsl/structure_tree.hpp>
#include <sdsl/util.hpp>

namespace sdsl
{
namespace detail
{

/*!\brief A bit compressed word
 * \tparam value_t The represented value_type.
 * \tparam bits_per_value How many bits are used to store one value. Must be less than 64.
 *
 * \details
 *
 * Uses bit compression to pack as many values as possible into one word.
 * The last bits won't be used if `bits_per_value` is not a power of two.
 */
template <typename value_t, size_t bits_per_value>
class bit_compressed_word
{
  private:
    static_assert(bits_per_value <= 64, "The maximum bit size is 64 for a value.");

    //!\brief The maximal number of values that can be stored in one word.
    static constexpr uint64_t max_size = (sizeof(uint64_t) << 3) / bits_per_value;
    //!\brief A mask used to zero out all bits that do not belong to the value.
    static constexpr uint64_t bit_mask = bits::lo_set[bits_per_value];

    //!\brief The data holder that stores the compressed values.
    uint64_t word{};

  public:
    //!\brief The size type needed for serialisation.
    using size_type = size_t;

    //!\brief The default constructor.
    bit_compressed_word() = default;
    //!\brief The copy constructor.
    bit_compressed_word(bit_compressed_word const &) = default;
    //!\brief The move constructor.
    bit_compressed_word(bit_compressed_word &&) = default;
    //!\brief The copy assignment.
    bit_compressed_word & operator=(bit_compressed_word const &) = default;
    //!\brief The move assignment.
    bit_compressed_word & operator=(bit_compressed_word &&) = default;
    //!\brief The destructor.
    ~bit_compressed_word() = default;

    /*!\brief Constructs from a range of values.
     * \tparam it_t The iterator type.
     * \param[in] it The iterator pointing to the first element to be stored.
     * \param[in] end The end of the range.
     *
     * \details
     *
     * The size of the range must be less or equal than `max_size`.
     */
    template <typename it_t>
    constexpr bit_compressed_word(it_t it, it_t end) noexcept
    {
        assign(it, end);
    }

    /*!\brief Extracts the value from the given index.
     * \param[in] index The index to get the value from.
     * \returns The value at given index.
     */
    constexpr value_t operator[](size_t const index) const noexcept
    {
        assert(index < max_size);
        uint64_t offset = index * bits_per_value;
        return static_cast<value_t>((word >> offset) & bit_mask);
    }

    /*!\brief Assigns a range to the word.
     * \copydetails sdsl::detail::bit_compressed_word::bit_compressed_word(it_t it, it_t end)
     */
    template <typename it_t>
    constexpr void assign(it_t it, it_t end) noexcept
    {
        assert(static_cast<uint64_t>(std::distance(it, end)) <= max_size);

        for (size_t index = 0; it != end; ++it, ++index)
        {
            uint64_t offset = index * bits_per_value;
            word = (word & ~(bit_mask << offset)) | uint64_t{ *it } << offset;
        }
    }

    //!\brief Implicitly converts to the word type.
    constexpr operator uint64_t() const noexcept { return word; }

    //!\brief Saves to the stream.
    size_type serialize(std::ostream & out, structure_tree_node * v = nullptr, const std::string name = "") const
    {
        structure_tree_node * child = structure_tree::add_child(v, name, sdsl::util::class_name(*this));
        size_type written_bytes = sdsl::serialize(word, out, child, "compressed_word");
        structure_tree::add_size(child, written_bytes);
        return written_bytes;
    }

    //!\brief Loads from the stream.
    void load(std::istream & in) { sdsl::load(word, in); }

    //!\brief Saves to the archive.
    template <typename archive_t>
    void CEREAL_SAVE_FUNCTION_NAME(archive_t & ar) const
    {
        ar(CEREAL_NVP(word));
    }

    //!\brief Loads from the archive.
    template <typename archive_t>
    void CEREAL_LOAD_FUNCTION_NAME(archive_t & ar)
    {
        ar(CEREAL_NVP(word));
    }
};
} // namespace detail
} // namespace sdsl

//! Namespace for the succinct data structure library.
namespace sdsl
{

/*!\brief A rank structure proposed by Christopher Pockrandt
 *
 * This data structure is similar to rank data structures on bit vectors.
 * It supports constant time rank and prefix_rank queries on int vectors.
 *
 * \tparam alphabet_size         Size of the alphabet represented in the int_vector, i.e., largest value + 1.
 * \tparam words_per_block       Words per block (equivalent to the number of popcount operations in the worst-case per
 * rank query). \tparam blocks_per_superblock Blocks per superblock.
 *
 * \par Reference
 *    Christopher Pockrandt:
 *    EPR-Dictionaries: A practical and fast data structure for constant time searches in unidirectional and
 *                      bidirectional FM-indices. WEA 2008: 154-168
 *
 * @ingroup rank_support_group
 */
template <uint8_t alphabet_size, uint8_t words_per_block = 1, uint8_t blocks_per_superblock = 4>
class rank_support_int_v : public rank_support_int<alphabet_size>
{
  private:
    //!\brief The type of the base class.
    using base_t = rank_support_int<alphabet_size>;

    // Import sigma specific constants from base class.
    using base_t::bits_per_word;
    using base_t::sigma;
    using base_t::sigma_bits;

    //!\brief How many values can be stored in one word.
    static constexpr uint64_t values_per_word{ 64ULL / sigma_bits };
    //!\brief How many values can be stored in one block.
    static constexpr uint64_t values_per_block{ words_per_block * values_per_word };
    //!\brief How many values can be stored in one superblock.
    static constexpr uint64_t values_per_superblock{ blocks_per_superblock * values_per_block };
    //!\brief How many words can be stored in one superblock.
    static constexpr uint64_t words_per_superblock{ words_per_block * blocks_per_superblock };
    //!\brief The effective alphabet size needed to compute the prefix ranks.
    static constexpr uint64_t effective_alphabet_size = alphabet_size - 1;

    struct superblock_entry;

    //!\brief The vector over the superblock superblocks.
    std::vector<superblock_entry> superblocks{};
    //!\brief The size of the original text.
    typename base_t::size_type text_size{};

  public:
    //!\brief The size type.
    using typename base_t::size_type;
    //!\brief The value type.
    using typename base_t::value_type;

    /*!\brief Constructs and initialises the rank support structure for the given text.
     * \param[in] text_ptr The pointer to the text to build the rank support for.
     *
     * \details
     *
     * The text will be copied into the superblock structure to utilise better cache locality when computing the
     * prefix rank for a given symbol and prefix length. Accordingly, the pointer to the text of the base class will
     * always be a nullptr.
     */
    explicit rank_support_int_v(const int_vector<> * text_ptr = nullptr)
      : rank_support_int<alphabet_size>(nullptr)
    {
        static_assert(blocks_per_superblock > 1, "There must be at least two blocks per superblock!");

        if (text_ptr == nullptr || text_ptr->empty()) return;

        text_size = text_ptr->size();

        // NOTE: number of elements is artificially increased by one because rank can be called on m_v[size()]
        uint64_t const word_count = (text_size + values_per_word - 1) / values_per_word;
        size_type const superblock_count = (word_count + words_per_superblock - 1) / words_per_superblock;

        // Buffers to keep track of the cumulative sums for the superblocks and blocks inside of a superblock.
        std::array<uint64_t, effective_alphabet_size> buf_blocks{};
        std::array<uint64_t, effective_alphabet_size> buf_superblocks{};

        // Allocate memory for the superblocks.
        superblocks.resize(superblock_count);

        // Iterate over the superblock entries and initialise them.
        auto text_slice_it = text_ptr->begin();
        uint64_t word_id = 0; // We basically iterate over all words of the underlying text.
        for (auto entry_it = superblocks.begin(); entry_it != superblocks.end(); ++entry_it)
        {
            // First initialise the superblock text.
            for (auto & compressed_word : entry_it->superblock_text)
            {
                // Get the text slice that can be stored in one word.
                auto text_slice_end = std::next(text_slice_it,
                                                std::min<size_t>(std::distance(text_slice_it, text_ptr->end()),
                                                                 values_per_word));
                compressed_word.assign(text_slice_it, text_slice_end); // Assign text slice to compressed word.
                text_slice_it = text_slice_end;                        // Set to next text slice begin.
            }

            // Second initialise the superblock counts.
            // The rank values are stored for every symbol of the alphabet in consecutive order.
            // The last symbol can be ignored since it's prefix sum will always be same as the prefix length.
            auto superblock_it = entry_it->superblocks.begin(); // Store the begin of the super block in the node.
            for (size_t letter_rank = 0; letter_rank < effective_alphabet_size; ++letter_rank, ++superblock_it)
            {
                buf_superblocks[letter_rank] += buf_blocks[letter_rank]; // Update sum with previous superblock
                *superblock_it = buf_superblocks[letter_rank];           // Store the counts.
                buf_blocks[letter_rank] = 0; // Reset the block counts for the next superblock.
            }

            // Third initialise the block counts:
            // The stored block counts represent the cumulative sum of the previous blocks in the super block.
            // The first block of the superblock is not stored explicitly since it has no predecessor.
            // A block stores the counts for the letters consecutive in memory from [0..max_letter] and starts then the
            // next block at offset `i * effective_alphabet_size`, where `i` is the current block id.
            // TODO: Make the implementation safe for multiple words per block
            auto text_it = entry_it->superblock_text.begin();
            for (auto block_it = entry_it->blocks.begin(); word_id < word_count && block_it != entry_it->blocks.end();
                 ++word_id, ++text_it)
            {
                // Get the prefix ranks for the current word for each letter and store them in the respective block
                for (size_t letter_rank = 0; letter_rank < effective_alphabet_size; ++letter_rank, ++block_it)
                {
                    buf_blocks[letter_rank] += base_t::full_word_prefix_rank(*text_it, letter_rank);
                    *block_it = buf_blocks[letter_rank];
                }
            }

            // Count the last block which is not stored explicitly.
            if (word_id < word_count)
            {
                for (uint64_t letter = 0; letter < effective_alphabet_size; ++letter)
                    buf_blocks[letter] += base_t::full_word_prefix_rank(*text_it, letter);

                ++word_id;
            }
        }
    }

    //!\brief Defaulted copy constructor.
    rank_support_int_v(const rank_support_int_v &) = default;
    //!\brief Defaulted move constructor.
    rank_support_int_v(rank_support_int_v &&) = default;
    //!\brief Defaulted copy assignment.
    rank_support_int_v & operator=(const rank_support_int_v &) = default;
    //!\brief Defaulted move assignment.
    rank_support_int_v & operator=(rank_support_int_v &&) = default;
    //!\brief Defaulted destructor.
    ~rank_support_int_v() = default;

    /*!\brief Counts the occurrences of v in the prefix [0..idx-1]
     * \param position The position of the symbol to get the prefix rank for (corresponds to length of the
     *                 prefix: `[0..position - 1]`).
     * \param v The alphabet symbol to get the rank for.
     * \sa prefix_rank
     */
    size_type rank(const size_type position, const value_type v) const
    {
        switch (v)
        {
            case 0: return prefix_rank_impl<false>(position, v);
            case sigma - 1: return position - prefix_rank_impl<false>(position, v - 1);
            default: return prefix_rank_impl<true>(position, v);
        }
    }

    //! \brief Alias for rank(position, v)
    inline size_type operator()(const size_type position, const value_type v) const { return rank(position, v); }

    /*!\brief Counts the occurrences of elements smaller or equal to v in the prefix [0..idx-1]
     * \param position The position of the symbol to get the prefix rank for (corresponds to length of the
     *                 prefix: `[0..position - 1]`).
     * \param v The alphabet symbol to get the rank for.
     * \sa rank
     */
    size_type prefix_rank(const size_type position, const value_type v) const
    {
        assert(position <= text_size);
        assert(v <= sigma);

        if (unlikely(v == sigma - 1)) return position;

        return prefix_rank_impl<false>(position, v);
        // TODO: Enable me!
        // compute in-block queries for all words before the in-block queries
        // this only applies when multiple words are in one block
        // if constexpr (words_per_block > 1)
        // {
        //     size_type const word_id{idx / values_per_word};
        //     uint64_t w{word_id - (word_id % words_per_block)};
        //     while (w < word_id)
        //     {
        //         res += this->full_word_prefix_rank(this->m_v->data(), w, v);
        //         ++w;
        //     }
        //     // std::cout << "res3=" << res << '\n';
        // }
    }

    //!\brief Returns the size of the original text.
    size_type size() const { return text_size; }

    /*!\brief Returns the text value at the given position.
     * \param[in] position The text position to get the value from.
     */
    value_type value_at(const size_type position) const
    {
        assert(position < text_size);
        return superblocks[to_superblock_position(position)].value_at(position);
    }

    //!\brief Saves to the stream.
    size_type serialize(std::ostream & out, structure_tree_node * v = nullptr, const std::string name = "") const
    {
        structure_tree_node * child = structure_tree::add_child(v, name, util::class_name(*this));
        size_type written_bytes = sdsl::serialize(superblocks, out, child, "superblocks_vector");
        written_bytes += write_member(text_size, out, child, "text_size");
        structure_tree::add_size(child, written_bytes);
        return written_bytes;
    }

    //!\brief Loads from the stream.
    void load(std::istream & in, const int_vector<> * /*v*/)
    {
        this->m_v = nullptr;
        sdsl::load(superblocks, in);
        read_member(text_size, in);
    }

    //! Equality operator.
    friend bool operator==(rank_support_int_v const & lhs, rank_support_int_v const & rhs) noexcept
    {
        return (lhs.superblocks == rhs.superblocks) && (lhs.text_size == rhs.text_size);
    }

    //! Inequality operator.
    friend bool operator!=(rank_support_int_v const & lhs, rank_support_int_v const & rhs) noexcept
    {
        return !(lhs == rhs);
    }

    //!\brief Saves to the archive.
    template <typename archive_t>
    void CEREAL_SAVE_FUNCTION_NAME(archive_t & ar) const
    {
        ar(CEREAL_NVP(superblocks));
        ar(CEREAL_NVP(text_size));
    }

    //!\brief Loads from the archive.
    template <typename archive_t>
    void CEREAL_LOAD_FUNCTION_NAME(archive_t & ar)
    {
        ar(CEREAL_NVP(superblocks));
        ar(CEREAL_NVP(text_size));
    }

    //!\brief Does nothing for the rank_support_int structure.
    void set_vector(const int_vector<> * /*other_text*/) {
    } // TODO: Check where this interface is needed, since it is dangerous?
      // I would be able to reset the text without recomputing the rank support structure which is in general a
      // bad design.

  private:
    /*!\brief Determines the superblock position covering the given text position.
     * \param[in] position The given text position.
     * \returns The position of the superblock that covers the given text position.
     */
    constexpr size_type to_superblock_position(size_t const position) const noexcept
    {
        return position / values_per_superblock;
    }

    /*!\brief Implements the prefix rank calculation.
     * \param position The position of the symbol to get the prefix rank for (corresponds to length of the
     *                 prefix: `[0..position - 1]`).
     * \param v The alphabet symbol to get the rank for.
     */
    template <bool compute_prefix_delta>
    size_type prefix_rank_impl(size_type const position, const value_type v) const
    {
        assert(position <= text_size);

        if (unlikely(text_size == 0)) // TODO: Maybe there could be some logic in the constructor for this case?
            return 0;

        superblock_entry const & entry = superblocks[to_superblock_position(position)];
        return entry.template superblock_rank<compute_prefix_delta>(v) +
               entry.template block_rank<compute_prefix_delta>(position, v) +
               entry.template in_block_rank<compute_prefix_delta>(position, v);

        // TODO: Enable me!
        // if constexpr (words_per_block > 1)
        // {
        //     size_type const word_id{position / values_per_word};
        //     uint64_t w{word_id - (word_id % words_per_block)};
        //     while (w < word_id)
        //     {
        //         res_upper += this->full_word_prefix_rank(this->m_v->data(), w, v);
        //         res_lower += this->full_word_prefix_rank(this->m_v->data(), w, v - 1);
        //         ++w;
        //     }
        // }
    }
};

/*!\brief Stores a superblock entry in a cache friendly pattern.
 *
 * \details
 *
 * One superblock entry represents one superblock in the rank support structure.
 * The text is stored efficiently in a sdsl::detail::bit_compressed_word.
 * The superblock array stores a rank count for `sigma - 1` many values.
 * The block array stores a rank count for `blocks_per_superblock - 1` many blocks times `sigma - 1` for every
 * symbol of the alphabet.
 */
template <uint8_t alphabet_size, uint8_t words_per_block, uint8_t blocks_per_superblock>
struct rank_support_int_v<alphabet_size, words_per_block, blocks_per_superblock>::superblock_entry
{
    using size_type = typename base_t::size_type;
    //!\brief The offset used to jump to the correct block position.
    static constexpr size_t block_offset = effective_alphabet_size;
    //!\brief How many bits needed to store the block ranks.
    static constexpr size_t bits_per_block_value = ceil_log2(values_per_superblock);
    //!\brief The smallest integer type needed to store the block ranks.
    using block_value_type = std::conditional_t<bits_per_block_value <= 8,
                                                uint8_t,
                                                std::conditional_t<bits_per_block_value <= 16, uint16_t, uint32_t>>;

    //!\brief The array storing the super block values.
    std::array<uint64_t, (alphabet_size - 1)> superblocks;
    //!\brief The array storing the block values.
    std::array<block_value_type, (blocks_per_superblock - 1) * (alphabet_size - 1)> blocks;
    //!\brief The array storing the bit compressed text.
    std::array<detail::bit_compressed_word<uint8_t, sigma_bits>, words_per_superblock> superblock_text;

    /*!\brief Returns the rank value from the superblock.
     * \tparam compute_prefix_delta A flag to indicate if the actual value or the delta with the previous symbol
     *                              shall be computed.
     * \param[in] v The symbol to get the rank for.
     */
    template <bool compute_prefix_delta>
    constexpr size_type superblock_rank(value_type const v) const noexcept
    {
        return superblocks[v] - ((compute_prefix_delta) ? superblocks[v - 1] : 0);
    }

    /*!\brief Returns the rank value from the block.
     * \tparam compute_prefix_delta A flag to indicate if the actual value or the delta with the previous symbol
     *                              shall be computed.
     * \param[in] position The text position to get the rank for.
     * \param[in] v The symbol to get the rank for.
     *
     * \details
     *
     * The first block stores the counts for the actual second block.
     * Hence, if we are in the first block of the superblock we get the first block value but multiply it with 0.
     */
    template <bool compute_prefix_delta>
    constexpr size_type block_rank(size_t const position, value_type const v) const noexcept
    {
        size_type const block_id = block_position_in_superblock(position);
        size_type const block_position = absolute_block_position(block_id) + v;
        return (block_id != 0) * (blocks[block_position] - ((compute_prefix_delta) ? blocks[block_position - 1] : 0));
    }

    /*!\brief Returns the rank value from the in-block query.
     * \tparam compute_prefix_delta A flag to indicate if the actual value or the delta with the previous symbol
     *                              shall be computed.
     * \param[in] position The text position to get the rank for.
     * \param[in] v The symbol to get the rank for.
     *
     * \details
     *
     * If the position is at the beginning of a block the compute rank value is multiplied with 0.
     */
    template <bool compute_prefix_delta>
    constexpr size_type in_block_rank(size_t const position, value_type const v) const noexcept
    {
        // First, get the local bit position within the data of the super block.
        size_type const bit_pos = absolute_bit_position(position);
        // Second, compute the word that contains this value.
        uint64_t const word = superblock_text[absolute_word_position(bit_pos)];
        // Third, compute the in-block rank given the current word.
        return (position % values_per_block != 0) * word_prefix_rank<compute_prefix_delta>(word, bit_pos, v);
    }

    /*!\brief Extracts the value at the given position.
     * \param position The position of the value to extract.
     */
    value_type value_at(size_type position) const noexcept
    {
        size_type bit_position = absolute_bit_position(position);
        return superblock_text[absolute_word_position(bit_position)][position % values_per_word];
    }

    //!\brief Saves to the stream.
    size_type serialize(std::ostream & out, structure_tree_node * v = nullptr, const std::string name = "") const
    {
        structure_tree_node * child = structure_tree::add_child(v, name, sdsl::util::class_name(*this));
        size_type written_bytes = 0;
        written_bytes += sdsl::serialize(superblocks.size(), out, child, "prefix_superblock_counts");
        for (const auto & x : superblocks) written_bytes += sdsl::serialize(x, out, child, "[]");

        written_bytes += sdsl::serialize(blocks.size(), out, child, "prefix_block_counts");
        for (const auto & x : blocks) written_bytes += sdsl::serialize(x, out, child, "[]");

        written_bytes += sdsl::serialize(superblock_text.size(), out, child, "superblock_text");
        for (const auto & x : superblock_text) written_bytes += sdsl::serialize(x, out, child, "[]");

        structure_tree::add_size(child, written_bytes);
        return written_bytes;
    }

    //!\brief Loads from the stream.
    void load(std::istream & in)
    {
        size_type array_size;
        sdsl::load(array_size, in);
        assert(array_size == superblocks.size());
        for (size_type idx = 0; idx < array_size; ++idx) sdsl::load(superblocks[idx], in);

        sdsl::load(array_size, in);
        assert(array_size == blocks.size());
        for (size_type idx = 0; idx < array_size; ++idx) sdsl::load(blocks[idx], in);

        sdsl::load(array_size, in);
        assert(array_size == superblock_text.size());
        for (size_type idx = 0; idx < array_size; ++idx) sdsl::load(superblock_text[idx], in);
    }

    //!\brief Equality operator.
    friend bool operator==(superblock_entry const & lhs, superblock_entry const & rhs) noexcept
    {
        return (lhs.superblocks == rhs.superblocks) && (lhs.blocks == rhs.blocks) &&
               (lhs.superblock_text == rhs.superblock_text);
    }

    //!\brief Inequality operator.
    friend bool operator!=(superblock_entry const & lhs, superblock_entry const & rhs) noexcept
    {
        return !(lhs == rhs);
    }

    //!\brief Saves to the archive.
    template <typename archive_t>
    void CEREAL_SAVE_FUNCTION_NAME(archive_t & ar) const
    {
        ar(CEREAL_NVP(superblocks));
        ar(CEREAL_NVP(blocks));
        ar(CEREAL_NVP(superblock_text));
    }

    //!\brief Loads from the archive.
    template <typename archive_t>
    void CEREAL_LOAD_FUNCTION_NAME(archive_t & ar)
    {
        ar(CEREAL_NVP(superblocks));
        ar(CEREAL_NVP(blocks));
        ar(CEREAL_NVP(superblock_text));
    }

  private:
    //!\brief Maps the given position to the block position inside of the superblock.
    static constexpr size_type block_position_in_superblock(size_t const position) noexcept
    { // if constexpr (blocks_per_superblock power of 2)
        return (position / values_per_block) % blocks_per_superblock;
    }

    //!\brief Maps a block position to its absolute position within the block array.
    static constexpr size_type absolute_block_position(size_t const block_position) noexcept
    {
        return (block_position + (block_position == 0) - 1) * block_offset;
    }

    //!\brief Maps a text position to the respective bit position in the bit vector.
    static constexpr size_type absolute_bit_position(size_t const position) noexcept
    {
        return (position % values_per_superblock) * sigma_bits;
    }

    //!\brief Maps a bit position to the word position within the superblock text array.
    static constexpr size_type absolute_word_position(size_t const bit_position) noexcept
    { // We don't care if it overflows as we protect against it later.
        return bit_position / bits_per_word;
    }

    //!\brief Computes the in-block rank for the delta prefix.
    template <bool compute_prefix_delta>
    static constexpr auto word_prefix_rank(const uint64_t word, const uint64_t bit_pos, const value_type v) ->
                                                      typename std::enable_if<compute_prefix_delta, size_type>::type
    {
        auto && prefix_rank = base_t::word_prefix_rank(word, bit_pos, v - 1, v);
        return prefix_rank[1] - prefix_rank[0];
    }

    //!\brief Computes the in-block rank for the non-delta prefix.
    template <bool compute_prefix_delta>
    static constexpr auto word_prefix_rank(const uint64_t word, const uint64_t bit_pos, const value_type v) ->
                                                      typename std::enable_if<!compute_prefix_delta, size_type>::type
    {
        return base_t::word_prefix_rank(word, bit_pos, v)[0];
    }
};

} // end namespace sdsl

#endif // end file
