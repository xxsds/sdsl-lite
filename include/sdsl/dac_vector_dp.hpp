// Copyright (c) 2026, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
/*!\file dac_vector_dp.hpp
 * \brief dac_vector_dp.hpp contains a variable-length integer vector whose levels are chosen by dynamic programming.
 */
#ifndef INCLUDED_SDSL_DAC_VECTOR_DP
#define INCLUDED_SDSL_DAC_VECTOR_DP

#include <array>
#include <cassert>
#include <cstdint>
#include <iosfwd>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include <sdsl/bits.hpp>
#include <sdsl/int_vector.hpp>
#include <sdsl/io.hpp>
#include <sdsl/iterators.hpp>
#include <sdsl/rank_support_v.hpp>
#include <sdsl/structure_tree.hpp>
#include <sdsl/util.hpp>

namespace sdsl
{

//! An immutable vector of unsigned integers stored in levels of increasing width.
/*! Each value is stored whole in the first level that is wide enough for it.
 *  A bit per value and level says whether the value continues to the next level,
 *  and a rank over these bits gives its position there.
 *  The level widths minimize the total size and are chosen by dynamic programming,
 *  as for the directly addressable codes in [1].
 *
 *  \par References
 *       [1] N. Brisaboa, S. Ladra and G. Navarro: ,,DACs: Bringing direct access to
 *           variable-length codes'', Information Processing and Management 49(1), 2013.
 *
 * \tparam t_bv         Bit vector for the continuation bits, with a rank_1_type.
 * \tparam t_max_levels Default maximum number of levels.
 */
template <typename t_bv = bit_vector, int t_max_levels = 64>
class dac_vector_dp
{
    static_assert(t_max_levels > 0, "dac_vector_dp: at least one level is needed");

public:
    using value_type = int_vector<>::value_type;
    using size_type = int_vector<>::size_type;
    using difference_type = ptrdiff_t;
    using const_iterator = random_access_const_iterator<dac_vector_dp>;
    using iterator = const_iterator;
    using const_reference = value_type const;
    using reference = const_reference;
    using index_category = iv_tag;
    using rank_type = typename t_bv::rank_1_type;

private:
    // Serialized in this order. The level of a value is found by following its continuation bits.
    t_bv m_next;                       // continuation bits of all levels, level after level
    rank_type m_next_rank;             // rank over m_next
    std::vector<int_vector<>> m_level; // values that end at each level
    std::vector<size_t> m_start;       // position of each level's first bit in m_next
    size_type m_size = 0;              // not serialized: the sum of the level sizes

    // Estimated bits for one level: its continuation bits and a fixed header and rank overhead.
    static constexpr double k_level_overhead = 128;

    // Values wider than `low` bits enter the level, and those up to `high` bits end there.
    // wider[k] counts the values wider than k bits, and wider[0] counts all values.
    static double level_bits(std::array<uint64_t, 66> const & wider, int low, int high)
    {
        return double(high) * (wider[low] - wider[high]) + wider[low] + k_level_overhead;
    }

    // The widths, in bits, of the widest value each level holds. The last one is the widest value.
    static std::vector<int> choose_widths(std::array<uint64_t, 66> const & wider, int max_width, int max_levels)
    {
        // cost[k][w] = least bits to store the values wider than w bits in at most k levels.
        // A width of max_width needs no further level.
        double const inf = std::numeric_limits<double>::max();
        std::vector<std::vector<double>> cost(max_levels + 1, std::vector<double>(max_width + 1, inf));
        std::vector<std::vector<int>> next(max_levels + 1, std::vector<int>(max_width + 1, -1));
        for (int k = 0; k <= max_levels; ++k)
            cost[k][max_width] = 0;
        for (int k = 1; k <= max_levels; ++k)
        {
            for (int low = 0; low < max_width; ++low)
            {
                for (int high = low + 1; high <= max_width; ++high)
                {
                    if (cost[k - 1][high] == inf)
                        continue;
                    double const bits = level_bits(wider, low, high) + cost[k - 1][high];
                    if (bits < cost[k][low])
                    {
                        cost[k][low] = bits;
                        next[k][low] = high;
                    }
                }
            }
        }
        std::vector<int> widths;
        for (int k = max_levels, low = 0; low < max_width; --k)
            widths.push_back(low = next[k][low]);
        return widths;
    }

    static int value_width(uint64_t x)
    {
        return x ? bits::hi(x) + 1 : 0;
    }

    void set_rank()
    {
        m_next_rank.set_vector(&m_next);
    }

public:
    dac_vector_dp() = default;

    // The rank structures are assigned, not copy-constructed: some declare only a copy assignment.
    dac_vector_dp(dac_vector_dp const & other) :
        m_next(other.m_next),
        m_level(other.m_level),
        m_start(other.m_start),
        m_size(other.m_size)
    {
        m_next_rank = other.m_next_rank;
        set_rank();
    }

    dac_vector_dp(dac_vector_dp && other) noexcept
    {
        *this = std::move(other);
    }

    dac_vector_dp & operator=(dac_vector_dp const & other)
    {
        if (this != &other)
            *this = dac_vector_dp(other);
        return *this;
    }

    dac_vector_dp & operator=(dac_vector_dp && other) noexcept
    {
        m_next = std::move(other.m_next);
        m_next_rank = std::move(other.m_next_rank);
        m_level = std::move(other.m_level);
        m_start = std::move(other.m_start);
        m_size = std::exchange(other.m_size, 0);
        set_rank();
        other.set_rank();
        return *this;
    }

    //! Builds the vector from a container of unsigned integers.
    /*! \param c          Any container with size() and operator[].
     *  \param max_levels Maximum number of levels.
     */
    template <class Container>
    dac_vector_dp(Container const & c, int max_levels = t_max_levels)
    {
        assert(max_levels > 0);
        m_size = c.size();

        std::array<uint64_t, 66> wider{};
        int max_width = 1; // a vector of zeros still needs one level
        for (size_type i = 0; i < m_size; ++i)
        {
            int const width = value_width(c[i]);
            ++wider[width];
            max_width = std::max(max_width, width);
        }
        // Turn the counts of each width into counts of the values wider than each width.
        uint64_t count = 0;
        for (int w = 64; w >= 0; --w)
            count += std::exchange(wider[w], count);
        wider[0] = m_size; // every value enters the first level

        std::vector<int> const widths = choose_widths(wider, max_width, max_levels);
        m_level.resize(widths.size());
        m_start.resize(widths.size());

        size_t next_bits = 0;
        for (size_t l = 0; l < widths.size(); ++l)
            next_bits += wider[l ? widths[l - 1] : 0];
        bit_vector next(next_bits, 0);

        // Values that reach the current level, in their original order.
        int_vector<> pending(m_size, 0, max_width);
        for (size_type i = 0; i < m_size; ++i)
            pending[i] = c[i];
        size_t bit = 0;
        for (size_t l = 0; l < widths.size(); ++l)
        {
            m_start[l] = bit;
            int_vector<> ending(pending.size(), 0, widths[l]);
            size_type ends = 0, continues = 0;
            for (size_type i = 0; i < pending.size(); ++i, ++bit)
            {
                uint64_t const value = pending[i];
                if (value_width(value) > widths[l])
                {
                    next[bit] = 1;
                    pending[continues++] = value;
                }
                else
                {
                    ending[ends++] = value;
                }
            }
            ending.resize(ends);
            util::bit_compress(ending);
            m_level[l] = std::move(ending);
            pending.resize(continues);
        }
        assert(pending.empty() && bit == next_bits);
        m_next = t_bv(std::move(next));
        m_next_rank = rank_type(&m_next);
    }

    //! The number of levels.
    size_t levels() const
    {
        return m_level.size();
    }

    size_type size() const
    {
        return m_size;
    }

    bool empty() const
    {
        return m_size == 0;
    }

    const_iterator begin() const
    {
        return const_iterator(this, 0);
    }

    const_iterator end() const
    {
        return const_iterator(this, size());
    }

    //! The i-th value.
    value_type operator[](size_type i) const
    {
        assert(i < size());
        size_t level = 0;
        // Within a level, i is the value's position among the values that reached it.
        while (m_next[m_start[level] + i])
        {
            i = m_next_rank(m_start[level] + i) - m_next_rank(m_start[level]);
            ++level;
        }
        i -= m_next_rank(m_start[level] + i) - m_next_rank(m_start[level]);
        return m_level[level][i];
    }

    size_type serialize(std::ostream & out, structure_tree_node * v = nullptr, std::string name = "") const
    {
        structure_tree_node * child = structure_tree::add_child(v, name, util::class_name(*this));
        size_type written_bytes = 0;
        written_bytes += m_next.serialize(out, child, "overflow");
        written_bytes += m_next_rank.serialize(out, child, "overflow_rank");
        written_bytes += sdsl::serialize(m_level, out, child, "data");
        written_bytes += sdsl::serialize(m_start, out, child, "offsets");
        structure_tree::add_size(child, written_bytes);
        return written_bytes;
    }

    void load(std::istream & in)
    {
        m_next.load(in);
        m_next_rank.load(in);
        set_rank();
        sdsl::load(m_level, in);
        sdsl::load(m_start, in);
        m_size = 0;
        for (auto const & level : m_level)
            m_size += level.size();
    }
};

} // namespace sdsl
#endif
