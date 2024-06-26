// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
/*!\file iterators.hpp
 * \brief iterators.hpp contains an generic iterator for random access containers.
 * \author Simon Gog
 */
#ifndef INCLUDED_SDSL_ITERATORS
#define INCLUDED_SDSL_ITERATORS

#include <iterator>
#include <type_traits> // std::invoke_result_t

#include <sdsl/int_vector.hpp>

namespace sdsl
{

//! Generic iterator for a random access container
/*!\tparam t_rac Type of random access container.
 */
template <class t_rac>
class random_access_const_iterator
{
public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = typename t_rac::value_type;
    using difference_type = typename t_rac::difference_type;
    using pointer = value_type *;
    using reference = value_type &;

    typedef const typename t_rac::value_type const_reference;
    typedef typename t_rac::size_type size_type;
    typedef random_access_const_iterator<t_rac> iterator;

private:
    t_rac const * m_rac; // pointer to the random access container
    typename t_rac::size_type m_idx;

    template <class t_RAC>
    friend typename random_access_const_iterator<t_RAC>::difference_type
    operator-(random_access_const_iterator<t_RAC> const & x, random_access_const_iterator<t_RAC> const & y);

public:
    //! Constructor
    random_access_const_iterator(t_rac const * rac, size_type idx = 0) : m_rac(rac), m_idx(idx)
    {}

    //! Dereference operator for the Iterator.
    const_reference operator*() const
    {
        return (*m_rac)[m_idx];
    }

    //! Prefix increment of the Iterator.
    iterator & operator++()
    {
        ++m_idx;
        return *this;
    }

    //! Postfix increment of the Iterator.
    iterator operator++(int)
    {
        random_access_const_iterator it = *this;
        ++(*this);
        return it;
    }

    //! Prefix decrement of the Iterator.
    iterator & operator--()
    {
        --m_idx;
        return *this;
    }

    //! Postfix decrement of the Iterator.
    iterator operator--(int)
    {
        random_access_const_iterator it = *this;
        --(*this);
        return it;
    }

    iterator & operator+=(difference_type i)
    {
        if (i < 0)
            return *this -= (-i);
        m_idx += i;
        return *this;
    }

    iterator & operator-=(difference_type i)
    {
        if (i < 0)
            return *this += (-i);
        m_idx -= i;
        return *this;
    }

    iterator operator+(difference_type i) const
    {
        iterator it = *this;
        return it += i;
    }

    iterator operator-(difference_type i) const
    {
        iterator it = *this;
        return it -= i;
    }

    const_reference operator[](difference_type i) const
    {
        return *(*this + i);
    }

    bool operator==(iterator const & it) const
    {
        return it.m_rac == m_rac && it.m_idx == m_idx;
    }

    bool operator!=(iterator const & it) const
    {
        return !(*this == it);
    }

    bool operator<(iterator const & it) const
    {
        return m_idx < it.m_idx;
    }

    bool operator>(iterator const & it) const
    {
        return m_idx > it.m_idx;
    }

    bool operator>=(iterator const & it) const
    {
        return !(*this < it);
    }

    bool operator<=(iterator const & it) const
    {
        return !(*this > it);
    }
};

template <class t_rac>
inline typename random_access_const_iterator<t_rac>::difference_type
operator-(random_access_const_iterator<t_rac> const & x, random_access_const_iterator<t_rac> const & y)
{
    return (typename random_access_const_iterator<t_rac>::difference_type)x.m_idx
         - (typename random_access_const_iterator<t_rac>::difference_type)y.m_idx;
}

template <class t_rac>
inline random_access_const_iterator<t_rac> operator+(typename random_access_const_iterator<t_rac>::difference_type n,
                                                     random_access_const_iterator<t_rac> const & it)
{
    return it + n;
}

template <typename t_F>
struct random_access_container
{
    typedef int_vector<>::size_type size_type;
    typedef int_vector<>::difference_type difference_type;
    typedef typename std::invoke_result_t<t_F, size_type> value_type;
    typedef random_access_const_iterator<random_access_container> iterator_type;

    t_F f;
    size_type m_size;

    random_access_container(){};
    random_access_container(t_F ff, size_type size) : f(ff), m_size(size)
    {}

    value_type operator[](size_type i) const
    {
        return f(i);
    }

    size_type size() const
    {
        return m_size;
    }

    iterator_type begin() const
    {
        return iterator_type(this, 0);
    }

    iterator_type end() const
    {
        return iterator_type(this, size());
    }
};

} // end namespace sdsl
#endif
