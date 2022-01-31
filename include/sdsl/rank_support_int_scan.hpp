// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
/*!\file rank_support_int_scan.hpp
 * \brief rank_support_int_scan.hpp contains rank_support_int_scan that support a sdsl::int_vector with linear time
 *        rank information.
 * \author Christopher Pockrandt
 */
#ifndef INCLUDED_SDSL_RANK_SUPPORT_INT_SCAN
#define INCLUDED_SDSL_RANK_SUPPORT_INT_SCAN

#include <sdsl/rank_support_int.hpp>

namespace sdsl
{

/*!\brief A class supporting rank queries in linear time.
 * \ingroup rank_support_group
 * \tparam alphabet_size Size of the alphabet used in the underlying sdsl::int_vector.
 *
 * \par Space complexity
 *       Constant.
 *  \par Time complexity
 *       Linear in the size of the supported vector.
 */

template <uint8_t alphabet_size>
class rank_support_int_scan : public rank_support_int<alphabet_size>
{
  private:
    using base_t = rank_support_int<alphabet_size>;

  public:
    typedef int_vector<> int_vector_type;
    typedef typename rank_support_int<alphabet_size>::size_type size_type;
    typedef typename rank_support_int<alphabet_size>::value_type value_type;

  public:
    explicit rank_support_int_scan(const int_vector<> * v = nullptr)
      : rank_support_int<alphabet_size>(v){};
    rank_support_int_scan(const rank_support_int_scan & rs) = default;
    rank_support_int_scan(rank_support_int_scan && rs) = default;
    rank_support_int_scan & operator=(const rank_support_int_scan & rs) = default;
    rank_support_int_scan & operator=(rank_support_int_scan && rs) = default;
    size_type rank(size_type idx, const value_type v) const;
    size_type operator()(size_type idx, const value_type v) const { return rank(idx, v); };
    size_type prefix_rank(size_type idx, const value_type v) const;
    size_type size() const { return this->m_v->size(); };
    size_type serialize(std::ostream & out, structure_tree_node * v = nullptr, const std::string name = "") const
    {
        return serialize_empty_object(out, v, name, this);
    }
    void load(std::istream &, const int_vector<> * v = nullptr)
    {
        this->m_v = v;
        this->init(v);
    }
    void set_vector(const int_vector<> * v = nullptr) { this->m_v = v; }
};

/*!\brief Counts the occurrences of v in the prefix [0..idx-1]
 * \param idx Argument for the length of the prefix v[0..idx-1].
 * \param v Argument which value to count.
 * \sa prefix_rank
 */
template <uint8_t alphabet_size>
inline typename rank_support_int_scan<alphabet_size>::size_type rank_support_int_scan<alphabet_size>::rank(
                                                  const size_type idx,
                                                  const value_type v) const
{
    assert(v < this->t_v);
    assert(this->m_v != nullptr);
    assert(idx <= this->m_v->size());

    if (unlikely(v == 0)) return prefix_rank(idx, v);

    const uint64_t * p = this->m_v->data();
    size_type i = 0;
    size_type result = 0;
    size_type word_pos = (idx * this->t_b) >> 6;
    while (i < word_pos)
    {
        result += base_t::full_word_rank(base_t::extract_word(p, i), v);
        ++i;
    }
    return result + base_t::word_rank(base_t::extract_word(p, idx), idx * this->sigma_bits, v);
}

/*!\brief Counts the occurrences of elements smaller or equal to v in the prefix [0..idx-1]
 * \param idx Argument for the length of the prefix v[0..idx-1].
 * \param v Argument which value (including smaller values) to count.
 * \sa rank
 */
template <uint8_t alphabet_size>
inline typename rank_support_int_scan<alphabet_size>::size_type rank_support_int_scan<alphabet_size>::prefix_rank(
                                                  const size_type idx,
                                                  const value_type v) const
{
    assert(v < this->t_v);
    assert(this->m_v != nullptr);
    assert(idx <= this->m_v->size());

    if (unlikely(v == this->t_v - 1)) return idx;

    const uint64_t * p = this->m_v->data();
    size_type word_pos = (idx * this->sigma_bits) >> 6;
    size_type i = 0;
    size_type result = 0;

    while (i < word_pos)
    {
        result += base_t::full_word_prefix_rank(base_t::extract_word(p, i), v);
        ++i;
    }

    return result + base_t::word_prefix_rank(base_t::extract_word(p, idx), idx * this->sigma_bits, v)[0];
}

} // namespace sdsl

#endif // end file
