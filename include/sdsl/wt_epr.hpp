// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
/*!\file wt_epr.hpp
 * \brief wt_epr.hpp contains a class for the EPR dictionary of byte sequences.
 *        The EPR-dictionary can be interpreted as a specialized wavelet tree of height 0.
 * \author Christopher Pockrandt
 */
#ifndef INCLUDED_SDSL_WT_EPR
#define INCLUDED_SDSL_WT_EPR

#include <assert.h>
#include <cmath>
#include <iosfwd>
#include <iterator>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <sdsl/cereal.hpp>
#include <sdsl/int_vector.hpp>
#include <sdsl/io.hpp>
#include <sdsl/iterators.hpp>
#include <sdsl/rank_support_int_v.hpp>
#include <sdsl/sdsl_concepts.hpp>
#include <sdsl/structure_tree.hpp>
#include <sdsl/util.hpp>
#include <sdsl/wt_helper.hpp>

//! Namespace for the succinct data structure library.
namespace sdsl
{

/*!\brief An EPR-dictionary based wavelet
 * \ingroup wt
 * \tparam alphabet_size Size of the alphabet.
 * \tparam t_rank        Rank support for pattern `1` on the bitvector.
 * \tparam t_tree_strat  Tree strategy determines alphabet and the tree class used to navigate the WT.
 */
template <uint8_t alphabet_size, class rank_type = rank_support_int_v<alphabet_size>, class t_tree_strat = byte_tree<>>
class wt_epr
{
public:
    typedef typename t_tree_strat::template type<wt_epr> tree_strat_type;
    typedef int_vector<>::size_type size_type;
    typedef int_vector<>::value_type value_type;
    typedef random_access_const_iterator<wt_epr> const_iterator;
    typedef const_iterator iterator;
    typedef typename int_vector<>::difference_type difference_type;
    typedef wt_tag index_category;
    typedef byte_alphabet_tag /*typename tree_strat_type::alphabet_category*/ alphabet_category;
    enum
    {
        lex_ordered = true
    };

private:
    //!\brief Check if underlying rank support structure stores the text implicitly.
    static constexpr bool has_inblock_text = std::is_same<rank_type, rank_support_int_v<alphabet_size>>::value;

    size_type m_size = 0;  //!< original text size
    size_type m_sigma = 0; //!< alphabet size
    int_vector<> m_bv;     //!< bit vector to store the wavelet tree
    rank_type m_bv_rank;   //!< rank support for the wavelet tree bit vector

    //!\brief Overload for the special epr rank structure.
    template <bool has_inblock_text_>
    auto construct_init_rank_select(int_vector<> intermediate_bitvector) -> std::enable_if_t<has_inblock_text_, void>
    {
        // The text is stored inside of the rank structure so we do not store it here.
        m_bv_rank = rank_type{&intermediate_bitvector}; // Create the rank support structure.
    }

    //!\brief Overload for the other rank support structures.
    template <bool has_inblock_text_>
    auto construct_init_rank_select(int_vector<> intermediate_bitvector) -> std::enable_if_t<!has_inblock_text_, void>
    {
        m_bv = std::move(intermediate_bitvector);
        m_bv_rank = rank_type{&m_bv}; // Create the rank support structure.
    }

    //!\brief Overload for the special epr rank structure. Extract the text value from the given position.
    template <bool has_inblock_text_>
    auto value_at(size_type const position) const -> std::enable_if_t<has_inblock_text_, value_type>
    { // In the special epr rank implementation, the text is stored in the superblocks.
        assert(position < size());
        return m_bv_rank.value_at(position); // Extract the value from the rank support structure.
    }

    //!\brief Overload for the other rank support structures.
    template <bool has_inblock_text_>
    auto value_at(size_type const position) const -> std::enable_if_t<!has_inblock_text_, value_type>
    {
        assert(position < size());
        return m_bv[position];
    }

public:
    size_type const & sigma = m_sigma;
    int_vector<> const & bv = m_bv;

    //!\brief Default constructor.
    wt_epr() = default;

    /*!\brief Construct the EPR-dictionary from a sequence defined by two iterators
     * \param begin Iterator to the start of the input.
     * \param end   Iterator one past the end of the input.
     * \par Time complexity
     *      \f$ \Order{n\log|\Sigma|}\f$, where \f$n=size\f$
     */
    template <typename t_it>
    wt_epr(t_it begin, t_it end) : m_size(std::distance(begin, end))
    {
        if (0 == m_size)
            return;

        // The largest letter, is the effective alphabet size
        for (auto it = begin; it != end; ++it)
        {
            size_t c = *it;
            // The text cannot have a character equal or larger than the required alphabet_size.
            if (c >= alphabet_size)
                throw std::domain_error{"The given text uses an alphabet that is larger than the explicitly given "
                                        "alphabet size."};
        }
        m_sigma = alphabet_size;

        // Generate wavelet tree bit sequence m_bv
        int_vector<> intermediate_bitvector{};
        intermediate_bitvector.width(std::ceil(std::log2(m_sigma)));
        intermediate_bitvector.resize(m_size);

        std::copy(begin, end, intermediate_bitvector.begin());

        // 5. Initialize rank and select data structures for m_bv
        construct_init_rank_select<has_inblock_text>(std::move(intermediate_bitvector));
    }

    template <typename t_it>
    wt_epr(t_it begin, t_it end, std::string) : wt_epr(begin, end)
    {}

    //!\brief Copy constructor
    wt_epr(wt_epr const & wt) : m_size(wt.m_size), m_sigma(wt.m_sigma), m_bv(wt.m_bv), m_bv_rank(wt.m_bv_rank)
    {
        m_bv_rank.set_vector(&m_bv);
    }

    wt_epr(wt_epr && wt) :
        m_size(wt.m_size),
        m_sigma(wt.m_sigma),
        m_bv(std::move(wt.m_bv)),
        m_bv_rank(std::move(wt.m_bv_rank))
    {
        m_bv_rank.set_vector(&m_bv);
    }

    //!\brief Assignment operator
    wt_epr & operator=(wt_epr const & wt)
    {
        if (this != &wt)
        {
            wt_epr tmp(wt);         // re-use copy-constructor
            *this = std::move(tmp); // re-use move-assignment
        }
        return *this;
    }

    //!\brief Move assignment operator
    wt_epr & operator=(wt_epr && wt)
    {
        if (this != &wt)
        {
            m_size = wt.m_size;
            m_sigma = wt.m_sigma;
            m_bv = std::move(wt.m_bv);
            m_bv_rank = std::move(wt.m_bv_rank);
            m_bv_rank.set_vector(&m_bv);
        }
        return *this;
    }

    //!\brief Returns the size of the original vector.
    size_type size() const
    {
        return m_size;
    }

    //!\brief Returns whether the wavelet tree contains no data.
    bool empty() const
    {
        return m_size == 0;
    }

    /*!\brief Recovers the i-th symbol of the original vector.
     * \param i Index in the original vector.
     * \return The i-th symbol of the original vector.
     * \par Time complexity
     *      \f$ \Order{1} \f$
     *
     * \par Precondition
     *      \f$ i < size() \f$
     */
    auto operator[](size_type const i) const
    {
        assert(i < size());
        return value_at<has_inblock_text>(i);
    };

    /*!\brief Calculates how many symbols c are in the prefix [0..i-1].
     * \param i Exclusive right bound of the range.
     * \param c Symbol c.
     * \return Number of occurrences of symbol c in the prefix [0..i-1].
     * \par Time complexity
     *      \f$ \Order{1} \f$
     *
     * \par Precondition
     *      \f$ i \leq size() \f$
     */
    size_type rank(size_type i, value_type c) const
    {
        assert(i <= size());
        return m_bv_rank.rank(i, c);
    };

    /*!\brief Calculates how many times symbol wt[i] occurs in the prefix [0..i-1].
     * \param i The index of the symbol.
     * \return  Pair (rank(wt[i],i),wt[i])
     * \par Time complexity
     *      \f$ \Order{1} \f$
     *
     * \par Precondition
     *      \f$ i < size() \f$
     */
    std::pair<size_type, value_type> inverse_select(size_type i) const
    {
        assert(i < size());
        value_type value = (*this)[i];
        return std::make_pair(m_bv_rank.rank(i, value), value);
    }

    // TODO: implement (if necessary?)
    /*!\brief For each symbol c in wt[i..j-1] get rank(i,c) and rank(j,c).
     * \param i        The start index (inclusive) of the interval.
     * \param j        The end index (exclusive) of the interval.
     * \param k        Reference for number of different symbols in [i..j-1].
     * \param cs       Reference to a vector that will contain in
     *                 cs[0..k-1] all symbols that occur in [i..j-1] in
     *                 arbitrary order (if lex_ordered = false) and ascending
     *                 order (if lex_ordered = true).
     * \param rank_c_i Reference to a vector which equals
     *                 rank_c_i[p] = rank(i,cs[p]), for \f$ 0 \leq p < k \f$.
     * \param rank_c_j Reference to a vector which equals
     *                 rank_c_j[p] = rank(j,cs[p]), for \f$ 0 \leq p < k \f$.
     *
     * \par Precondition
     *      \f$ i \leq j \leq size() \f$
     *      \f$ cs.size() \geq \sigma \f$
     *      \f$ rank_{c_i}.size() \geq \sigma \f$
     *      \f$ rank_{c_j}.size() \geq \sigma \f$
     */
    // void interval_symbols(size_type       i,
    //        size_type       j,
    //        size_type&      k,
    //        std::vector<value_type>& cs,
    //        std::vector<size_type>&  rank_c_i,
    //        std::vector<size_type>&  rank_c_j) const
    // { }

    /*!\brief How many symbols are lexicographic smaller/greater than c in [i..j-1].
     * \param i       Start index (inclusive) of the interval.
     * \param j       End index (exclusive) of the interval.
     * \param c       Symbol c.
     * \return A triple containing:
     *         * rank(i,c)
     *         * #symbols smaller than c in [i..j-1]
     *         * #symbols greater than c in [i..j-1]
     *
     * \par Precondition
     *       \f$ i \leq j \leq size() \f$
     */
    template <class t_ret_type = std::tuple<size_type, size_type, size_type>>
    t_ret_type lex_count(size_type i, size_type j, value_type c) const
    {
        assert(i <= j and j <= size());
        // size_type smaller = 0;
        // size_type greater = (j - i) - (m_bv_rank.prefix_rank(j, c) - m_bv_rank.prefix_rank(i, c));
        // if (c > 0)
        //     smaller = m_bv_rank.prefix_rank(j, c-1) - m_bv_rank.prefix_rank(i, c-1);
        // size_type rank = m_bv_rank.rank(i, c);

        // TODO: write a function returning a pair for (i, c) and (i, c-1) and benchmark!
        size_type smaller = 0;
        size_type prefix_i_c = m_bv_rank.prefix_rank(i, c);
        size_type prefix_i_c_1 = 0;
        size_type greater = j - i - m_bv_rank.prefix_rank(j, c) + prefix_i_c;
        if (c > 0)
        {
            prefix_i_c_1 = m_bv_rank.prefix_rank(i, c - 1);
            smaller = m_bv_rank.prefix_rank(j, c - 1) - prefix_i_c_1;
        }
        size_type rank = prefix_i_c - prefix_i_c_1;

        return t_ret_type{rank, smaller, greater};
    }

    /*\brief How many symbols are lexicographic smaller than c in [0..i-1].
     * \param i Exclusive right bound of the range.
     * \param c Symbol c.
     * \return A tuple containing:
     *         * rank(i,c)
     *         * #symbols smaller than c in [0..i-1]
     * \par Precondition
     *       \f$ i \leq size() \f$
     * \note
     * This method is only available if lex_ordered = true
     */
    template <class t_ret_type = std::tuple<size_type, size_type>>
    t_ret_type lex_smaller_count(size_type i, value_type c) const
    {
        assert(i <= size());
        // TODO: write a function returning a pair for (i, c) and (i, c-1) and benchmark!
        size_type prefix_count_smaller = 0;
        if (c > 0)
            prefix_count_smaller = m_bv_rank.prefix_rank(i, c - 1);
        return t_ret_type{m_bv_rank.prefix_rank(i, c) - prefix_count_smaller, prefix_count_smaller};
    }

    //!\brief Returns a const_iterator to the first element.
    const_iterator begin() const
    {
        return const_iterator(this, 0);
    }

    //!\brief Returns a const_iterator to the element after the last element.
    const_iterator end() const
    {
        return const_iterator(this, size());
    }

    //!\brief Serializes the data structure into the given ostream
    size_type serialize(std::ostream & out, structure_tree_node * v = nullptr, std::string name = "") const
    {
        structure_tree_node * child = structure_tree::add_child(v, name, util::class_name(*this));
        size_type written_bytes = 0;
        written_bytes += write_member(m_size, out, child, "size");
        written_bytes += write_member(m_sigma, out, child, "sigma");
        written_bytes += m_bv.serialize(out, child, "bv");
        written_bytes += m_bv_rank.serialize(out, child, "bv_rank");
        structure_tree::add_size(child, written_bytes);
        return written_bytes;
    }

    //!\brief Loads the data structure from the given istream.
    void load(std::istream & in)
    {
        read_member(m_size, in);
        read_member(m_sigma, in);
        m_bv.load(in);
        m_bv_rank.load(in, &m_bv);
    }

    //!\brief Equality operator.
    friend bool operator==(wt_epr const & lhs, wt_epr const & rhs) noexcept
    {
        return (lhs.m_size == rhs.m_size) && (lhs.m_sigma == rhs.m_sigma) && (lhs.m_bv == rhs.m_bv)
            && (lhs.m_bv_rank == rhs.m_bv_rank);
    }

    //!\brief Inequality operator.
    friend bool operator!=(wt_epr const & lhs, wt_epr const & rhs) noexcept
    {
        return !(lhs == rhs);
    }

    template <typename archive_t>
    void CEREAL_SAVE_FUNCTION_NAME(archive_t & ar) const
    {
        ar(CEREAL_NVP(m_size));
        ar(CEREAL_NVP(m_sigma));
        ar(CEREAL_NVP(m_bv));
        ar(CEREAL_NVP(m_bv_rank));
    }

    template <typename archive_t>
    void CEREAL_LOAD_FUNCTION_NAME(archive_t & ar)
    {
        ar(CEREAL_NVP(m_size));
        ar(CEREAL_NVP(m_sigma));
        ar(CEREAL_NVP(m_bv));
        ar(CEREAL_NVP(m_bv_rank));
        m_bv_rank.set_vector(&m_bv);
    }
};
} // namespace sdsl

#endif
