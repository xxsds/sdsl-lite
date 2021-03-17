// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
/*!\file int_vector.hpp
 * \brief int_vector.hpp contains the sdsl::int_vector class.
 * \author Simon Gog
 */
#ifndef INCLUDED_SDSL_INT_VECTOR
#define INCLUDED_SDSL_INT_VECTOR

#include <cassert>
#include <cinttypes>
#include <cstddef>
#include <cstdlib>
#include <cstring> // for memcpy
#include <ctime>   // for rand initialization
#include <ios>
#include <iosfwd>   // forward declaration of ostream
#include <iostream> // for cerr
#include <istream>
#include <iterator>
#include <ostream>
#include <stdexcept> // for exceptions
#include <string>
#include <typeinfo>
#include <vector>

#include <sdsl/bits.hpp>
#include <sdsl/config.hpp>
#include <sdsl/io.hpp>
#include <sdsl/memory_management.hpp>
#include <sdsl/ram_fs.hpp>
#include <sdsl/sfstream.hpp>
#include <sdsl/structure_tree.hpp>
#include <sdsl/uintx_t.hpp>
#include <sdsl/util.hpp>

#include <initializer_list>
#include <type_traits>

//! Namespace for the succinct data structure library.
namespace sdsl
{

typedef uint64_t std_size_type_for_int_vector;

template <uint8_t t_width = 0>
class int_vector;

template <class int_vector_type>
class mm_item;

//! bit_vector is a specialization of the int_vector.
typedef int_vector<1> bit_vector;

template <class t_int_vector>
class int_vector_reference;

template <class t_int_vector>
class int_vector_iterator_base;

template <class t_int_vector>
class int_vector_iterator;

template <class t_int_vector>
class int_vector_const_iterator;

template <uint8_t t_width, std::ios_base::openmode t_mode>
class int_vector_mapper;

template <uint8_t b, uint8_t t_patter_len> // forward declaration
class rank_support_v;

class rank_support;

class select_support;

template <uint8_t t_bit_pattern, uint8_t t_pattern_len>
class select_support_mcl;

namespace coder
{
template <typename T>
class fibonacci;
template <typename T>
class elias_delta;
template <typename T>
class elias_gamma;
template <uint8_t t_width>
class comma;
} // namespace coder

template <uint8_t t_width>
struct int_vec_category_trait
{
    typedef iv_tag type;
};

template <>
struct int_vec_category_trait<1>
{
    typedef bv_tag type;
};

template <uint8_t t_width>
struct int_vector_trait
{
    typedef uint64_t value_type;
    typedef int_vector<t_width> int_vector_type;
    typedef int_vector_reference<int_vector_type> reference;
    typedef uint64_t const_reference;
    typedef uint8_t int_width_type;
    typedef int_vector_iterator<int_vector_type> iterator;
    typedef int_vector_const_iterator<int_vector_type> const_iterator;

    static iterator begin(int_vector_type * v, uint64_t *) noexcept { return iterator(v, 0); }
    static iterator end(int_vector_type * v, uint64_t *, int_vector_size_type) noexcept
    {
        return iterator(v, v->size() * v->width());
    }
    static const_iterator begin(const int_vector_type * v, const uint64_t *) noexcept { return const_iterator(v, 0); }
    static const_iterator end(const int_vector_type * v, const uint64_t *, int_vector_size_type) noexcept
    {
        return const_iterator(v, v->size() * v->width());
    }

    static void set_width(uint8_t new_width, int_width_type & width) noexcept
    {
        if (t_width == 0)
        {
            if (0 < new_width and new_width <= 64)
                width = new_width;
            else
                width = 64;
        }
    }
};

template <>
struct int_vector_trait<64>
{
    typedef uint64_t value_type;
    typedef int_vector<64> int_vector_type;
    typedef uint64_t & reference;
    typedef uint64_t const_reference;
    typedef uint8_t int_width_type;
    typedef uint64_t * iterator;
    typedef const uint64_t * const_iterator;

    static iterator begin(int_vector_type *, uint64_t * begin) noexcept { return begin; }
    static iterator end(int_vector_type *, uint64_t * begin, int_vector_size_type size) noexcept
    {
        return begin + size;
    }
    static const_iterator begin(const int_vector_type *, const uint64_t * begin) noexcept { return begin; }
    static const_iterator end(const int_vector_type *, const uint64_t * begin, int_vector_size_type size) noexcept
    {
        return begin + size;
    }

    static void set_width(uint8_t, int_width_type) noexcept {}
};

template <>
struct int_vector_trait<32>
{
    typedef uint32_t value_type;
    typedef int_vector<32> int_vector_type;
    typedef uint32_t & reference;
    typedef uint32_t const_reference;
    typedef uint8_t int_width_type;
    typedef uint32_t * iterator;
    typedef const uint32_t * const_iterator;

    static iterator begin(int_vector_type *, uint64_t * begin) noexcept { return (uint32_t *)begin; }
    static iterator end(int_vector_type *, uint64_t * begin, int_vector_size_type size) noexcept
    {
        return ((uint32_t *)begin) + size;
    }
    static const_iterator begin(const int_vector_type *, const uint64_t * begin) noexcept { return (uint32_t *)begin; }
    static const_iterator end(const int_vector_type *, const uint64_t * begin, int_vector_size_type size) noexcept
    {
        return ((uint32_t *)begin) + size;
    }

    static void set_width(uint8_t, int_width_type) noexcept {}
};

template <>
struct int_vector_trait<16>
{
    typedef uint16_t value_type;
    typedef int_vector<16> int_vector_type;
    typedef uint16_t & reference;
    typedef uint16_t const_reference;
    typedef uint8_t int_width_type;
    typedef uint16_t * iterator;
    typedef const uint16_t * const_iterator;

    static iterator begin(int_vector_type *, uint64_t * begin) noexcept { return (uint16_t *)begin; }
    static iterator end(int_vector_type *, uint64_t * begin, int_vector_size_type size) noexcept
    {
        return ((uint16_t *)begin) + size;
    }
    static const_iterator begin(const int_vector_type *, const uint64_t * begin) noexcept { return (uint16_t *)begin; }
    static const_iterator end(const int_vector_type *, const uint64_t * begin, int_vector_size_type size) noexcept
    {
        return ((uint16_t *)begin) + size;
    }

    static void set_width(uint8_t, int_width_type) noexcept {}
};

template <>
struct int_vector_trait<8>
{
    typedef uint8_t value_type;
    typedef int_vector<8> int_vector_type;
    typedef uint8_t & reference;
    typedef uint8_t const_reference;
    typedef uint8_t int_width_type;
    typedef uint8_t * iterator;
    typedef const uint8_t * const_iterator;

    static iterator begin(int_vector_type *, uint64_t * begin) noexcept { return (uint8_t *)begin; }
    static iterator end(int_vector_type *, uint64_t * begin, int_vector_size_type size) noexcept
    {
        return ((uint8_t *)begin) + size;
    }
    static const_iterator begin(const int_vector_type *, const uint64_t * begin) noexcept { return (uint8_t *)begin; }
    static const_iterator end(const int_vector_type *, const uint64_t * begin, int_vector_size_type size) noexcept
    {
        return ((uint8_t *)begin) + size;
    }

    static void set_width(uint8_t, int_width_type) noexcept {}
};

//! A generic vector class for integers of width \f$w\in [1..64]\f$.
/*!\author Simon Gog
 *
 *    This generic vector class could be used to generate a vector
 *    that contains integers of fixed width \f$w\in [1..64]\f$.
 *    It has a growth factor of 1.5 to achieve amortized running time.
 *    Note that resize() does not reserve more space than necessary.
 *
 *  \tparam t_width Width of the integer. If set to `0` it is variable
 *          during runtime, otherwise fixed at compile time.
 *  @ingroup int_vector
 */
template <uint8_t t_width>
class int_vector
{
  private:
    static_assert(t_width <= 64, "int_vector: width of must be at most 64bits.");

  public:
    typedef typename int_vector_trait<t_width>::value_type value_type;
    typedef typename int_vector_trait<t_width>::iterator iterator;
    typedef typename int_vector_trait<t_width>::const_iterator const_iterator;
    typedef typename int_vector_trait<t_width>::reference reference;
    typedef typename int_vector_trait<t_width>::const_reference const_reference;
    typedef int_vector_reference<int_vector> * pointer;
    typedef const value_type * const_pointer;
    typedef ptrdiff_t difference_type;
    typedef int_vector_size_type size_type;
    typedef typename int_vector_trait<t_width>::int_width_type int_width_type;
    typedef rank_support_v<1, 1> rank_1_type;
    typedef rank_support_v<0, 1> rank_0_type;
    typedef select_support_mcl<1, 1> select_1_type;
    typedef select_support_mcl<0, 1> select_0_type;
    typedef typename int_vec_category_trait<t_width>::type index_category;

    friend struct int_vector_trait<t_width>;
    friend class int_vector_iterator_base<int_vector>;
    friend class int_vector_iterator<int_vector>;
    friend class int_vector_const_iterator<int_vector>;
    template <uint8_t, std::ios_base::openmode>
    friend class int_vector_mapper;
    template <typename T>
    friend class coder::elias_delta;
    template <typename T>
    friend class coder::elias_gamma;
    template <typename T>
    friend class coder::fibonacci;
    template <uint8_t>
    friend class coder::comma;
    friend class memory_manager;
    static constexpr uint8_t fixed_int_width = t_width;
    float growth_factor = 1.5; //!< Growth factor for amortized constant time operations
                               // see the explanation in the documentation of FBVector on different growth factors
                               // https://github.com/facebook/folly/blob/master/folly/docs/FBVector.md#memory-handling

  private:
    size_type m_size;       //!< Number of bits needed to store int_vector.
    size_type m_capacity;   //!< Number of bits reserved by int_vector.
    uint64_t * m_data;      //!< Pointer to the memory for the bits.
    int_width_type m_width; //!< Width of the integers.

    // Hidden, since number of bits (size) does not go well together with int value.
    void bit_resize(const size_type size, const value_type value);

    void amortized_resize(const size_type size)
    {
        assert(growth_factor > 1.0);
        size_type bit_size = size * m_width;
        if (bit_size > m_capacity || m_data == nullptr)
        {
            // start with 64 bit if vector has no capacity
            size_type tmp_capacity = m_capacity == 0 ? 64 : m_capacity;
            // find smallest x s.t. m_capacity * 1.5 ** x >= size
            auto resize_factor = pow(growth_factor,
                                     std::ceil(std::log((bit_size + tmp_capacity - 1) / tmp_capacity) /
                                               std::log(growth_factor)));
            size_type new_capacity = std::ceil(tmp_capacity * resize_factor);
            memory_manager::resize(*this, new_capacity);
        }
        m_size = size * m_width;
    }

    // The number of 64-bit words used by the int_vector.
    size_type bit_data_size() const { return (m_size + 63) >> 6; }

  public:
    //! Constructor for int_vector.
    /*!\param size          Number of elements. Default value is 0.
     * \param default_value Initialize all value to `default value`.
     * \param int_width     The width of each integer.
     * \sa resize, width
     */

    int_vector(size_type size, value_type default_value, uint8_t int_width = t_width);

    //! Constructor to fix possible comparison with integeres issue.
    explicit int_vector(size_type size = 0)
      : int_vector(size, static_cast<value_type>(0), t_width)
    {}
    //! Constructor for initializer_list.
    int_vector(std::initializer_list<value_type> il)
      : int_vector(0, 0)
    {
        assign(il);
    }

    //! Constructor for iterator range
    /*!\param first Iterator pointing to first element to be copied.
     * \param last  Iterator pointing to the element behind the last one to be copied.
     */
    template <typename input_iterator_t>
    int_vector(typename std::enable_if<std::is_base_of<std::input_iterator_tag,
                                                       typename std::iterator_traits<input_iterator_t>::iterator_category>::
                                                                                         value,
                                       input_iterator_t>::type first,
               input_iterator_t last)
      : int_vector(0, 0)
    {
        assign(first, last);
    }

    //! Clearing the int_vector. Allocated memory will not be released.
    /*!\sa resize
     */
    void clear() noexcept { m_size = 0; }

    //! Remove element that iterator is pointing to.
    /*!\param it Iterator pointing to an element in int_vector
     */
    iterator erase(const_iterator it)
    {
        iterator it_nonconst = begin() + (it - cbegin());
        std::copy(it_nonconst + 1, end(), it_nonconst);
        resize(size() - 1);
        return it_nonconst;
    }

    //! Remove elements in given iterator range.
    /*!\param first Iterator pointing to first element to be deleted.
     * \param last  Iterator pointing to the elemnt after the one to be deleted.
     */
    iterator erase(const_iterator first, const_iterator last)
    {
        iterator first_nonconst = begin() + (first - cbegin());
        iterator last_nonconst = begin() + (last - cbegin());
        std::copy(last_nonconst, end(), first_nonconst);
        resize(size() - (last - first));
        return first_nonconst;
    }

    //! Insert an element constructed with std::forward<Args>(args) before the element that the iterator is pointing to.
    /*!\param it   Iterator pointing to an element in int_vector.
     * \param args Function parameter pack.
     */
    template <class... Args>
    iterator emplace(const_iterator it, Args &&... args)
    {
        return insert(it, 1, value_type(std::forward<Args>(args)...));
    }

    //! Insert an element before the element that the iterator is pointing to.
    /*!\param it    Iterator pointing to an element in int_vector.
     * \param value Element to be inserted.
     */
    iterator insert(const_iterator it, value_type value) { return insert(it, 1, value); }

    //! Insert n copies of an element before the element that the iterator is pointing to.
    /*!\param it    Iterator pointing to an element in int_vector.
     * \param n     Number of copies.
     * \param value Element to be inserted.
     */
    iterator insert(const_iterator it, size_type n, value_type value)
    {
        size_type pos = it - cbegin();
        amortized_resize(size() + n);
        iterator it_new = begin() + pos;
        std::copy_backward(it_new, end() - n, end());
        std::fill_n(it_new, n, value);
        return it_new;
    }

    //! Insert elements from intializer_list before the element that the iterator is pointing to.
    /*!\param it Iterator pointing to an element in int_vector.
     * \param il Elements to be inserted.
     */
    iterator insert(const_iterator it, std::initializer_list<value_type> il)
    {
        return insert(it, il.begin(), il.end());
    }

    //! Insert elements from an iterator pair before the element that the iterator `it` is pointing to.
    /*!\param it    Iterator pointing to an element in int_vector.
     * \param first Iterator pointing to first element to be inserted.
     * \param last  Iterator pointing to the elemnt after the one to be inserted.
     */
    template <typename input_iterator_t>
    typename std::enable_if<std::is_base_of<std::input_iterator_tag,
                                            typename std::iterator_traits<input_iterator_t>::iterator_category>::value,
                            iterator>::type
    insert(const_iterator it, input_iterator_t first, input_iterator_t last)
    {
        size_type pos = it - cbegin();
        amortized_resize(size() + last - first);
        iterator it_new = begin() + pos;
        std::copy_backward(it_new, end() - (last - first), end());
        std::copy(first, last, it_new);
        return it_new;
    }

    //! Returns first element.
    reference front() noexcept { return *begin(); }

    //! Returns first element.
    const_reference front() const noexcept { return *cbegin(); }

    //! Returns last element.
    reference back() noexcept { return *(end() - 1); }

    //! Returns last element.
    const_reference back() const noexcept { return *(cend() - 1); }

    //! Insert an element constructed with std::forward<Args>(args) at the end.
    /*!\param args Function parameter pack.
     */
    template <class... Args>
    void emplace_back(Args &&... args)
    {
        push_back(value_type(std::forward<Args>(args)...));
    }

    //! Insert element at the end.
    /*!\param value Element to be inserted.
     */
    void push_back(value_type value)
    {
        amortized_resize(size() + 1);
        *(end() - 1) = value;
    }

    //! Remove element at the end.
    void pop_back() { resize(size() - 1); }

    //! Move constructor.
    int_vector(int_vector && v);

    //! Copy constructor.
    int_vector(const int_vector & v);

    //! Destructor.
    ~int_vector();

    //! Assign. Resize int_vector to `size` and fill elements with `default_value`.
    /*!\param size Number of elements.
     * \param default_value Elements to be inserted.
     */
    void assign(size_type size, value_type default_value)
    {
        bit_resize(size * m_width);
        util::set_to_value(*this, default_value); // new initialization
    }

    //! Assign. Resize int_vector and initialize with initializer_list.
    /*!\param il Initializer_list.
     */
    void assign(std::initializer_list<value_type> il)
    {
        bit_resize(il.size() * m_width);
        size_type idx = 0;
        for (auto x : il) { (*this)[idx++] = x; }
    }

    //! Assign. Resize int_vector and initialize by copying from an iterator range.
    /*!\param first Iterator pointing to first element to be inserted.
     * \param last  Iterator pointing to the elemnt after the one to be inserted.
     */
    template <typename input_iterator_t>
    void assign(input_iterator_t first, input_iterator_t last)
    {
        assert(first <= last);
        bit_resize((last - first) * m_width);
        size_type idx = 0;
        while (first < last) { (*this)[idx++] = *(first++); }
    }

    //! Equivalent to size() == 0.
    bool empty() const noexcept { return 0 == m_size; }

    //! Swap method for int_vector.
    void swap(int_vector & v) noexcept { std::swap(v, *this); }

    //! Free unused allocated memory.
    void shrink_to_fit() { memory_manager::resize(*this, m_size); }

    //! Reserve storage. If the new capacity is smaller than the current, this method does nothing.
    /*!\param capacity New capacity in bits
     */
    void reserve(size_type capacity)
    {
        if (capacity * m_width > m_capacity || m_data == nullptr) { memory_manager::resize(*this, capacity * m_width); }
    }

    //! Resize the int_vector in terms of elements. If the current size is smaller than `size`, the additional elements
    //! are initialized with 0.
    /*! Only as much space as necessary is being allocated.
     * \param size Number of elements.
     */
    void resize(const size_type size) { resize(size, 0); }

    //! Resize the int_vector in terms of elements. Only as much space as necessary is allocated.
    /*!\param size The size to resize the int_vector in terms of elements.
     * \param value If the current size is smaller than `size`, the additional elements are initialized with value.
     */
    void resize(const size_type size, const value_type value) { bit_resize(size * m_width, value); }

    //! Resize the int_vector in terms of bits. Only as much space as necessary is allocated.
    /*!\param size The size to resize the int_vector in terms of bits.
     */
    void bit_resize(const size_type size);

    //! The number of elements in the int_vector.
    /*!\sa max_size, bit_size, capacity, bit_capacity
     */
    inline size_type size() const noexcept;

    //! Maximum size of the int_vector.
    /*!\sa size, bit_size, capacity, bit_capacity
     */
    static size_type max_size() noexcept { return ((size_type)1) << (sizeof(size_type) * 8 - 6); }

    //! The number of bits in the int_vector.
    /*!\sa size, max_size, bit_size, capacity
     */
    size_type bit_size() const noexcept { return m_size; }

    //! Returns the size of the occupied bits of the int_vector.
    /*! The capacity of a int_vector is greater or equal to the
     * size of the vector: capacity() >= size().
     * \sa size, bit_size, max_size, capacity, bit_capacity
     */
    inline size_type capacity() const noexcept;

    //! Returns the size of the occupied bits of the int_vector.
    /*! The bit_capacity of a int_vector is greater or equal to the
     * bit_size of the vector: bit_capacity() >= bit_size().
     * \sa size, bit_size, max_size, capacity, bit_capacity
     */
    size_type bit_capacity() const noexcept { return m_capacity; }

    //! Pointer to the raw data of the int_vector
    /*!\returns Const pointer to the raw data of the int_vector
     */
    const uint64_t * data() const noexcept { return m_data; }

    //! Pointer to the raw data of the int_vector
    /*!\returns pointer to the raw data of the int_vector
     */
    uint64_t * data() noexcept { return m_data; }

    //! Get the integer value of the binary string of length len starting at position idx in the int_vector.
    /*!\param idx Starting index of the binary representation of the integer.
     * \param len Length of the binary representation of the integer. Default value is 64.
     * \returns The integer value of the binary string of length len starting at position idx.
     * \sa setInt, getBit, setBit
     */
    value_type get_int(size_type idx, const uint8_t len = 64) const;

    //! Set the bits from position idx to idx+len-1 to the binary representation of integer x.
    /*! The bit at position idx represents the least significant bit(lsb), and the bit at
     * position idx+len-1 the most significant bit (msb) of x.
     * \param idx Starting index of the binary representation of x.
     * \param x   The integer to store in the int_vector.
     * \param len The length used to store x in the int_vector. Default value is 64.
     * \sa getInt, getBit, setBit
     */
    void set_int(size_type idx, value_type x, const uint8_t len = 64);

    //! Returns the width of the integers which are accessed via the [] operator.
    /*!\returns The width of the integers which are accessed via the [] operator.
     * \sa width
     */
    uint8_t width() const noexcept { return m_width; }

    //! Sets the width of the integers which are accessed via the [] operator, if t_width equals 0.
    /*!\param new_width New width of the integers accessed via the [] operator.
     * \note This method has no effect if t_width is in the range [1..64].
     * \sa width
     */
    void width(uint8_t new_width) noexcept { int_vector_trait<t_width>::set_width(new_width, m_width); }

    // Write data (without header) to a stream.
    size_type write_data(std::ostream & out) const;

    //! Serializes the int_vector to a stream.
    /*!\return The number of bytes written to out.
     *  \sa load
     */
    size_type serialize(std::ostream & out, structure_tree_node * v = nullptr, std::string name = "") const;

    //! Load the int_vector for a stream.
    void load(std::istream & in);

    /* For cereal we need to define different versions of the load and save function depending on whether we want
     * binary data or text (XML/JSON) data.
     * See https://github.com/USCiLab/cereal/blob/master/include/cereal/types/vector.hpp for an example.
     */

    //!\brief Serialise (save) via cereal if archive is not binary
    template <typename archive_t>
    inline typename std::enable_if<!cereal::traits::is_output_serializable<cereal::BinaryData<int_vector<t_width>>,
                                                                           archive_t>::value,
                                   void>::type
    CEREAL_SAVE_FUNCTION_NAME(archive_t & ar) const;

    //!\brief Serialise (save) via cereal if archive is binary
    template <typename archive_t>
    inline typename std::enable_if<cereal::traits::is_output_serializable<cereal::BinaryData<int_vector<t_width>>,
                                                                          archive_t>::value,
                                   void>::type
    CEREAL_SAVE_FUNCTION_NAME(archive_t & ar) const;

    //!\brief Serialise (load) via cereal if archive is not binary
    template <typename archive_t>
    inline typename std::enable_if<!cereal::traits::is_input_serializable<cereal::BinaryData<int_vector<t_width>>,
                                                                          archive_t>::value,
                                   void>::type
    CEREAL_LOAD_FUNCTION_NAME(archive_t & ar);

    //!\brief Serialise (save) via cereal if archive is binary
    template <typename archive_t>
    inline typename std::enable_if<cereal::traits::is_input_serializable<cereal::BinaryData<int_vector<t_width>>,
                                                                         archive_t>::value,
                                   void>::type
    CEREAL_LOAD_FUNCTION_NAME(archive_t & ar);

    //! non const version of [] operator
    /*!\param i Index the i-th integer of length width().
     *  \return A reference to the i-th integer of length width().
     */
    inline reference operator[](const size_type & i) noexcept;

    //! const version of [] operator
    /*!\param i Index the i-th integer of length width().
     *  \return The value of the i-th integer of length width().
     */
    inline const_reference operator[](const size_type & i) const noexcept;

    //! non const version of at() function
    /*!\param i Index the i-th integer of length width().
     *  \return A reference to the i-th integer of length width().
     */
    reference at(const size_type & i) { return (*this)[i]; }

    //! const version of at() function
    /*!\param i Index the i-th integer of length width().
     *  \return The value of the i-th integer of length width().
     */
    const_reference at(const size_type & i) const { return (*this)[i]; }

    //! Assignment operator.
    /*!\param v The vector v which should be assigned
     */
    int_vector & operator=(const int_vector & v);

    //! Move assignment operator.
    int_vector & operator=(int_vector && v);

    //! Equality operator for two int_vectors.
    /*! Two int_vectors are equal if
     *    - sizes are equal and
     *    - its elements are equal.
     */
    bool operator==(const int_vector<t_width> & v) const noexcept
    {
        if (bit_size() != v.bit_size()) return false;
        if (empty()) return true;
        const uint64_t * data1 = v.data();
        const uint64_t * data2 = data();
        for (size_type i = 0; i < bit_data_size() - 1; ++i)
        {
            if (*(data1++) != *(data2++)) return false;
        }
        uint8_t l = 64 - ((bit_data_size() << 6) - m_size);
        return ((*data1) & bits::lo_set[l]) == ((*data2) & bits::lo_set[l]);
    }

    //! Equality operator for an arbitrary container.
    /*! Note that this function is slow since it compares element by element
     * and cannot compare the bit representations of the containers.
     * Two containers are equal if
     *    - sizes are equal and
     *    - its elements are equal.
     */
    template <class container>
    friend bool operator==(const int_vector<t_width> & lhs, const container & rhs) noexcept
    {
        return std::equal(lhs.begin(), lhs.end(), rhs.begin());
    }

    //! Inequality operator for two int_vectors.
    /*! Two int_vectors are not equal if
     *    - sizes are not equal or
     *    - its elements are not equal.
     * Note that comparing two int_vectors of different widths is slow
     * since it compares element by element and not the bit representations of the int_vectors.
     */
    template <uint8_t t_width2>
    bool operator!=(const int_vector<t_width2> & v) const noexcept
    {
        return !(*this == v);
    }

    //! Less operator for two int_vectors
    /*! int_vector w is less than v if
     *    - w[i]==v[i] for i<j and w[j]<v[j] with j in [0, min(w.size(), v.size()) )
     *    - or w[i]==v[i] for all i < min(w.size(), v.size()) and w.size()<v.size().
     *  \sa operator>
     */
    bool operator<(const int_vector & v) const noexcept;

    //! Greater operator for two int_vectors
    /*! int_vector w is greater than v if
     *    - w[i]==v[i] for i<j and w[j]>v[j] with j in [0, min(w.size(), v.size()) )
     *    - or w[i]==v[i] for all i < min(w.size(), v.size()) and w.size()>v.size().
     */
    bool operator>(const int_vector & v) const noexcept;

    //! Less or equal operator
    bool operator<=(const int_vector & v) const noexcept;

    //! Greater of equal operator
    bool operator>=(const int_vector & v) const noexcept;

    //! bitwise-and-update operator
    int_vector & operator&=(const int_vector & v);

    //! bitwise-or-update equal operator
    int_vector & operator|=(const int_vector & v);

    //! bitwise-xor-update operator
    int_vector & operator^=(const int_vector & v);

    //! Iterator that points to the first element of the int_vector.
    /*!  Time complexity guaranty is O(1).
     */
    iterator begin() noexcept { return int_vector_trait<t_width>::begin(this, m_data); }

    //! Iterator that points to the element after the last element of int_vector.
    /*! Time complexity guaranty is O(1).
     */
    iterator end() noexcept { return int_vector_trait<t_width>::end(this, m_data, (m_size / m_width)); }

    //! Const iterator that points to the first element of the int_vector.
    const_iterator begin() const noexcept { return int_vector_trait<t_width>::begin(this, m_data); }

    //! Const iterator that points to the element after the last element of int_vector.
    const_iterator end() const noexcept { return int_vector_trait<t_width>::end(this, m_data, (m_size / m_width)); }

    //! Const iterator that points to the first element of the int_vector.
    const_iterator cbegin() const noexcept { return int_vector_trait<t_width>::begin(this, m_data); }

    //! Const iterator that points to the element after the last element of int_vector.
    const_iterator cend() const noexcept { return int_vector_trait<t_width>::end(this, m_data, (m_size / m_width)); }

    //! Flip all bits of bit_vector
    void flip()
    {
        static_assert(1 == t_width, "int_vector: flip() is available only for bit_vector.");
        if (!empty())
        {
            for (uint64_t i = 0; i < bit_data_size(); ++i) { m_data[i] = ~m_data[i]; }
        }
    }

    //! Read the size and int_width of a int_vector
    static size_t read_header(int_vector_size_type & size, int_width_type & int_width, std::istream & in)
    {
        uint64_t width_and_size = 0;
        read_member(width_and_size, in);
        size = width_and_size & bits::lo_set[56];
        uint8_t read_int_width = (uint8_t)(width_and_size >> 56);
        if (t_width == 0) { int_width = read_int_width; }
        if (t_width > 0 and t_width != read_int_width)
        {
            std::cerr << "Warning: Width of int_vector<" << (size_t)t_width << ">";
            std::cerr << " was specified as " << (size_type)read_int_width << std::endl;
            std::cerr << "Length is " << size << " bits" << std::endl;
        }
        return sizeof(width_and_size);
    }

    //! Write the size and int_width of a int_vector
    static uint64_t write_header(uint64_t size, uint8_t int_width, std::ostream & out)
    {
        if (t_width > 0)
        {
            if (t_width != int_width)
            {
                std::cout << "Warning: writing width=" << (size_type)int_width << " != fixed " << (size_type)t_width
                          << std::endl;
            }
        }
        uint64_t width_and_size = (((uint64_t)int_width) << 56) | size;
        return write_member(width_and_size, out);
    }

    struct raw_wrapper
    {
        const int_vector & vec;
        raw_wrapper() = delete;
        raw_wrapper(const int_vector & _vec)
          : vec(_vec)
        {}

        size_type serialize(std::ostream & out, structure_tree_node * v = nullptr, std::string name = "") const
        {
            structure_tree_node * child = structure_tree::add_child(v, name, util::class_name(*this));
            auto written_bytes = vec.write_data(out);
            structure_tree::add_size(child, written_bytes);
            return written_bytes;
        }
    };

    const raw_wrapper raw = raw_wrapper(*this);
};

//! A proxy class that acts as a reference to an integer of length \p len bits in a int_vector.
/*!\tparam t_int_vector The specific int_vector class.
 */
template <class t_int_vector>
class int_vector_reference
{
  public:
    typedef typename t_int_vector::value_type value_type;

  private:
    typename t_int_vector::value_type * const m_word;
    const uint8_t m_offset;
    const uint8_t m_len; //!< Length of the integer referred to in bits.
  public:
    //! Default constructor explicitly deleted.
    int_vector_reference() = delete;
    //! Copy and move explicitly defaulted.
    constexpr int_vector_reference(int_vector_reference const &) noexcept = default;
    constexpr int_vector_reference(int_vector_reference &&) noexcept = default;

    //! Constructor for the reference class
    /*!\param word Pointer to the corresponding 64bit word in the int_vector.
     * \param offset Offset to the starting bit (offset in [0..63])
     * \param len length of the integer, should be v->width()!!!
     */
    int_vector_reference(value_type * word, uint8_t offset, uint8_t len) noexcept
      : m_word(word)
      , m_offset(offset)
      , m_len(len){};

    //! Assignment operator for the proxy class
    /*!
     * The integer x is assign to the referenced
     * position in the t_int_vector with the specified width
     * of the int_vector
     * \param x 64bit integer to assign
     * \return A const_reference to the assigned reference
     */
    int_vector_reference & operator=(value_type x) noexcept
    {
        bits::write_int(m_word, x, m_offset, m_len);
        return *this;
    };

    int_vector_reference & operator=(const int_vector_reference & x) noexcept { return *this = value_type(x); };

    int_vector_reference & operator=(int_vector_reference && x) noexcept { return *this = value_type(std::move(x)); };

    //! Cast the reference to a int_vector<>::value_type
    operator value_type() const noexcept { return bits::read_int(m_word, m_offset, m_len); }

    //! Prefix increment of the proxy object
    int_vector_reference & operator++() noexcept
    {
        value_type x = bits::read_int(m_word, m_offset, m_len);
        bits::write_int(m_word, x + 1, m_offset, m_len);
        return *this;
    }

    //! Postfix increment of the proxy object
    value_type operator++(int) noexcept
    {
        value_type val = (typename t_int_vector::value_type) * this;
        ++(*this);
        return val;
    }

    //! Prefix decrement of the proxy object
    int_vector_reference & operator--() noexcept
    {
        value_type x = bits::read_int(m_word, m_offset, m_len);
        bits::write_int(m_word, x - 1, m_offset, m_len);
        return *this;
    }

    //! Postfix decrement of the proxy object
    value_type operator--(int) noexcept
    {
        value_type val = (value_type) * this;
        --(*this);
        return val;
    }

    //! Add assign from the proxy object
    int_vector_reference & operator+=(const value_type x) noexcept
    {
        value_type w = bits::read_int(m_word, m_offset, m_len);
        bits::write_int(m_word, w + x, m_offset, m_len);
        return *this;
    }

    //! Subtract assign from the proxy object
    int_vector_reference & operator-=(const value_type x) noexcept
    {
        value_type w = bits::read_int(m_word, m_offset, m_len);
        bits::write_int(m_word, w - x, m_offset, m_len);
        return *this;
    }

    bool operator==(const int_vector_reference & x) const noexcept { return value_type(*this) == value_type(x); }

    bool operator<(const int_vector_reference & x) const noexcept { return value_type(*this) < value_type(x); }
};

// For C++11
template <class t_int_vector>
inline void swap(int_vector_reference<t_int_vector> x, int_vector_reference<t_int_vector> y) noexcept
{
    // TODO: more efficient solution?
    typename int_vector_reference<t_int_vector>::value_type tmp = x;
    x = y;
    y = tmp;
}

// For C++11
template <class t_int_vector>
inline void swap(typename int_vector_reference<t_int_vector>::value_type & x,
                 int_vector_reference<t_int_vector> y) noexcept
{
    // TODO: more efficient solution?
    typename int_vector_reference<t_int_vector>::value_type tmp = x;
    x = y;
    y = tmp;
}

// For C++11
template <class t_int_vector>
inline void swap(int_vector_reference<t_int_vector> x,
                 typename int_vector_reference<t_int_vector>::value_type & y) noexcept
{
    // TODO: more efficient solution?
    typename int_vector_reference<t_int_vector>::value_type tmp = x;
    x = y;
    y = tmp;
}

// specialization for int_vector_reference for int_vector == bit_vector
// special thanks to Timo Beller, who pointed out that the specialization is missing
// Same implementation as in stl_bvector.h.
template <>
class int_vector_reference<bit_vector>
{
  public:
    typedef bool value_type;

  private:
    uint64_t * const m_word;
    uint64_t m_mask;

  public:
    //! Default constructor explicitly deleted.
    int_vector_reference() = delete;
    //! Copy and move explicitly defaulted.
    constexpr int_vector_reference(int_vector_reference const &) noexcept = default;
    constexpr int_vector_reference(int_vector_reference &&) noexcept = default;

    //! Constructor for the reference class
    /*!\param word Pointer to the corresponding 64bit word in the int_vector.
     * \param offset Offset to the starting bit (offset in [0..63])
     */
    int_vector_reference(uint64_t * word, uint8_t offset, uint8_t) noexcept
      : m_word(word)
      , m_mask(1ULL << offset){};

    //! Assignment operator for the proxy class
    int_vector_reference & operator=(bool x) noexcept
    {
        if (x)
            *m_word |= m_mask;
        else
            *m_word &= ~m_mask;
        return *this;
    };

    int_vector_reference & operator=(const int_vector_reference & x) noexcept { return *this = bool(x); };
    int_vector_reference & operator=(int_vector_reference && x) noexcept { return *this = bool(x); };

    //! Cast the reference to a bool
    operator bool() const noexcept { return !!(*m_word & m_mask); }

    bool operator==(const int_vector_reference & x) const noexcept { return bool(*this) == bool(x); }

    bool operator<(const int_vector_reference & x) const noexcept { return !bool(*this) && bool(x); }
};

// For C++11
template <>
inline void swap(int_vector_reference<bit_vector> x, int_vector_reference<bit_vector> y) noexcept
{
    // TODO: more efficient solution?
    bool tmp = x;
    x = y;
    y = tmp;
}

// For C++11
template <>
inline void swap(bool & x, int_vector_reference<bit_vector> y) noexcept
{
    // TODO: more efficient solution?
    bool tmp = x;
    x = y;
    y = tmp;
}

// For C++11
template <>
inline void swap(int_vector_reference<bit_vector> x, bool & y) noexcept
{
    // TODO: more efficient solution?
    bool tmp = x;
    x = y;
    y = tmp;
}

template <class t_int_vector>
class int_vector_iterator_base
  : public std::iterator<std::random_access_iterator_tag,
                         typename t_int_vector::value_type,
                         typename t_int_vector::difference_type>
{
  public:
    typedef uint64_t size_type;

  protected:
    uint8_t m_offset;
    uint8_t m_len;

  public:
    int_vector_iterator_base(uint8_t offset, uint8_t len)
      : m_offset(offset)
      , m_len(len)
    {}

    int_vector_iterator_base(const t_int_vector * v = nullptr, size_type idx = 0)
      : m_offset(idx & 0x3F)
      , m_len(v == nullptr ? 0 : v->m_width)
    {}
};

template <class t_int_vector>
class int_vector_iterator : public int_vector_iterator_base<t_int_vector>
{
  public:
    typedef int_vector_reference<t_int_vector> reference;
    typedef uint64_t value_type;
    typedef int_vector_iterator iterator;
    typedef reference * pointer;
    typedef typename t_int_vector::size_type size_type;
    typedef typename t_int_vector::difference_type difference_type;

    friend class int_vector_const_iterator<t_int_vector>;

  private:
    using int_vector_iterator_base<t_int_vector>::m_offset; // make m_offset easy usable
    using int_vector_iterator_base<t_int_vector>::m_len;    // make m_len easy usable

    typename t_int_vector::value_type * m_word;

  public:
    int_vector_iterator(t_int_vector * v = nullptr, size_type idx = 0)
      : int_vector_iterator_base<t_int_vector>(v, idx)
      , m_word((v != nullptr) ? v->m_data + (idx >> 6) : nullptr)
    {}

    int_vector_iterator(const int_vector_iterator<t_int_vector> & it)
      : int_vector_iterator_base<t_int_vector>(it)
      , m_word(it.m_word)
    {
        m_offset = it.m_offset;
        m_len = it.m_len;
    }

    reference operator*() const { return reference(m_word, m_offset, m_len); }

    //! Prefix increment of the Iterator
    iterator & operator++()
    {
        m_offset += m_len;
        if (m_offset >= 64)
        {
            m_offset &= 0x3F;
            ++m_word;
        }
        return *this;
    }

    //! Postfix increment of the Iterator
    iterator operator++(int)
    {
        int_vector_iterator it = *this;
        ++(*this);
        return it;
    }

    //! Prefix decrement of the Iterator
    iterator & operator--()
    {
        m_offset -= m_len;
        if (m_offset >= 64)
        {
            m_offset &= 0x3F;
            --m_word;
        }
        return *this;
    }

    //! Postfix decrement of the Iterator
    iterator operator--(int)
    {
        int_vector_iterator it = *this;
        --(*this);
        return it;
    }

    iterator & operator+=(difference_type i)
    {
        if (i < 0) return *this -= (-i);
        difference_type t = i * m_len;
        m_word += (t >> 6);
        if ((m_offset += (t & 0x3F)) & ~0x3F)
        {                     // if new offset is >= 64
            ++m_word;         // add one to the word
            m_offset &= 0x3F; // offset = offset mod 64
        }
        return *this;
    }

    iterator & operator-=(difference_type i)
    {
        if (i < 0) return *this += (-i);
        difference_type t = i * m_len;
        m_word -= (t >> 6);
        if ((m_offset -= (t & 0x3F)) & ~0x3F)
        { // if new offset is < 0
            --m_word;
            m_offset &= 0x3F;
        }
        return *this;
    }

    iterator & operator=(const int_vector_iterator<t_int_vector> & it)
    {
        if (this != &it)
        {
            m_word = it.m_word;
            m_offset = it.m_offset;
            m_len = it.m_len;
        }
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

    reference operator[](difference_type i) const { return *(*this + i); }

    bool operator==(const int_vector_iterator & it) const noexcept
    {
        return it.m_word == m_word && it.m_offset == m_offset;
    }

    bool operator!=(const int_vector_iterator & it) const noexcept { return !(*this == it); }

    bool operator<(const int_vector_iterator & it) const noexcept
    {
        if (m_word == it.m_word) return m_offset < it.m_offset;
        return m_word < it.m_word;
    }

    bool operator>(const int_vector_iterator & it) const noexcept
    {
        if (m_word == it.m_word) return m_offset > it.m_offset;
        return m_word > it.m_word;
    }

    bool operator>=(const int_vector_iterator & it) const noexcept { return !(*this < it); }

    bool operator<=(const int_vector_iterator & it) const noexcept { return !(*this > it); }
    inline difference_type operator-(const int_vector_iterator & it) const noexcept
    {
        return (((m_word - it.m_word) << 6) + m_offset - it.m_offset) / m_len;
    }
};

// template<class t_int_vector>
// void swap(const int_vector_iterator<t_int_vector> &x, const int_vector_iterator<t_int_vector> &y){
//  x->swap(*y);
//}

template <class t_int_vector>
inline int_vector_iterator<t_int_vector> operator+(typename int_vector_iterator<t_int_vector>::difference_type n,
                                                   const int_vector_iterator<t_int_vector> & it)
{
    return it + n;
}

template <class t_int_vector>
class int_vector_const_iterator : public int_vector_iterator_base<t_int_vector>
{
  public:
    typedef typename t_int_vector::value_type const_reference;
    typedef const typename t_int_vector::value_type * pointer;
    typedef int_vector_const_iterator const_iterator;
    typedef typename t_int_vector::size_type size_type;
    typedef typename t_int_vector::difference_type difference_type;

    template <class X>
    friend typename int_vector_const_iterator<X>::difference_type operator-(const int_vector_const_iterator<X> & x,
                                                                            const int_vector_const_iterator<X> & y);
    friend class int_vector_iterator<t_int_vector>;
    friend class int_vector_iterator_base<t_int_vector>;

  private:
    using int_vector_iterator_base<t_int_vector>::m_offset; // make m_offset easy usable
    using int_vector_iterator_base<t_int_vector>::m_len;    // make m_len easy usable

    const typename t_int_vector::value_type * m_word;

  public:
    int_vector_const_iterator(const t_int_vector * v = nullptr, size_type idx = 0)
      : int_vector_iterator_base<t_int_vector>(v, idx)
      , m_word((v != nullptr) ? v->m_data + (idx >> 6) : nullptr)
    {}

    int_vector_const_iterator(const int_vector_iterator<t_int_vector> & it)
      : m_word(it.m_word)
    {
        m_offset = it.m_offset;
        m_len = it.m_len;
    }

    const_reference operator*() const
    {
        if (m_offset + m_len <= 64) { return ((*m_word) >> m_offset) & bits::lo_set[m_len]; }
        return ((*m_word) >> m_offset) | ((*(m_word + 1) & bits::lo_set[(m_offset + m_len) & 0x3F]) << (64 - m_offset));
    }

    //! Prefix increment of the Iterator
    const_iterator & operator++()
    {
        m_offset += m_len;
        if (m_offset >= 64)
        {
            m_offset &= 0x3F;
            ++m_word;
        }
        return *this;
    }

    //! Postfix increment of the Iterator
    const_iterator operator++(int)
    {
        int_vector_const_iterator it = *this;
        ++(*this);
        return it;
    }

    //! Prefix decrement of the Iterator
    const_iterator & operator--()
    {
        m_offset -= m_len;
        if (m_offset >= 64)
        {
            m_offset &= 0x3F;
            --m_word;
        }
        return *this;
    }

    //! Postfix decrement of the Iterator
    const_iterator operator--(int)
    {
        int_vector_const_iterator it = *this;
        --(*this);
        return it;
    }

    const_iterator & operator+=(difference_type i)
    {
        if (i < 0) return *this -= (-i);
        difference_type t = i * m_len;
        m_word += (t >> 6);
        if ((m_offset += (t & 0x3F)) & ~0x3F)
        {                     // if new offset >= 64
            ++m_word;         // add one to the word
            m_offset &= 0x3F; // offset = offset mod 64
        }
        return *this;
    }

    const_iterator & operator-=(difference_type i)
    {
        if (i < 0) return *this += (-i);
        difference_type t = i * m_len;
        m_word -= (t >> 6);
        if ((m_offset -= (t & 0x3F)) & ~0x3F)
        { // if new offset is < 0
            --m_word;
            m_offset &= 0x3F;
        }
        return *this;
    }

    const_iterator operator+(difference_type i) const
    {
        const_iterator it = *this;
        return it += i;
    }

    const_iterator operator-(difference_type i) const
    {
        const_iterator it = *this;
        return it -= i;
    }

    const_reference operator[](difference_type i) const { return *(*this + i); }

    bool operator==(const int_vector_const_iterator & it) const noexcept
    {
        return it.m_word == m_word && it.m_offset == m_offset;
    }

    bool operator!=(const int_vector_const_iterator & it) const noexcept { return !(*this == it); }

    bool operator<(const int_vector_const_iterator & it) const noexcept
    {
        if (m_word == it.m_word) return m_offset < it.m_offset;
        return m_word < it.m_word;
    }

    bool operator>(const int_vector_const_iterator & it) const noexcept
    {
        if (m_word == it.m_word) return m_offset > it.m_offset;
        return m_word > it.m_word;
    }

    bool operator>=(const int_vector_const_iterator & it) const noexcept { return !(*this < it); }

    bool operator<=(const int_vector_const_iterator & it) const noexcept { return !(*this > it); }
};

template <class t_int_vector>
inline typename int_vector_const_iterator<t_int_vector>::difference_type operator-(
                                                  const int_vector_const_iterator<t_int_vector> & x,
                                                  const int_vector_const_iterator<t_int_vector> & y)
{
    return (((x.m_word - y.m_word) << 6) + x.m_offset - y.m_offset) / x.m_len;
}

template <class t_int_vector>
inline int_vector_const_iterator<t_int_vector> operator+(
                                                  typename int_vector_const_iterator<t_int_vector>::difference_type n,
                                                  const int_vector_const_iterator<t_int_vector> & it)
{
    return it + n;
}

template <class t_bv>
inline typename std::enable_if<std::is_same<typename t_bv::index_category, bv_tag>::value, std::ostream &>::type
operator<<(std::ostream & os, const t_bv & bv)
{
    for (auto b : bv) { os << b; }
    return os;
}

// ==== int_vector implementation  ====

template <uint8_t t_width>
inline int_vector<t_width>::int_vector(size_type size, value_type default_value, uint8_t int_width)
  : m_size(0)
  , m_capacity(0)
  , m_data(nullptr)
  , m_width(t_width)
{
    width(int_width);
    assign(size, default_value);
}

template <uint8_t t_width>
inline int_vector<t_width>::int_vector(int_vector && v)
  : m_size(v.m_size)
  , m_capacity(v.m_capacity)
  , m_data(v.m_data)
  , m_width(v.m_width)
{
    v.m_data = nullptr; // ownership of v.m_data now transfered
    v.m_size = 0;
    v.m_capacity = 0;
}

template <uint8_t t_width>
inline int_vector<t_width>::int_vector(const int_vector & v)
  : m_size(0)
  , m_capacity(0)
  , m_data(nullptr)
  , m_width(v.m_width)
{
    width(v.m_width);
    resize(v.size());
    if (v.m_size > 0)
    {
        if (memcpy(m_data, v.data(), bit_data_size() << 3) == nullptr)
        {
            throw std::bad_alloc(); // LCOV_EXCL_LINE
        }
    }
}

template <uint8_t t_width>
int_vector<t_width> & int_vector<t_width>::operator=(const int_vector & v)
{
    if (this != &v)
    { // if v is not the same object
        int_vector<t_width> tmp(v);
        *this = std::move(tmp);
    }
    return *this;
}

template <uint8_t t_width>
int_vector<t_width> & int_vector<t_width>::operator=(int_vector && v)
{
    if (this != &v)
    {                                 // if v is not the same object
        memory_manager::clear(*this); // clear allocated memory
        m_size = v.m_size;
        m_data = v.m_data; // assign new memory
        m_width = v.m_width;
        m_capacity = v.m_capacity;
        v.m_data = nullptr;
        v.m_size = 0;
        v.m_capacity = 0;
    }
    return *this;
}

// Destructor
template <uint8_t t_width>
int_vector<t_width>::~int_vector()
{
    memory_manager::clear(*this);
}

// sdsl::swap (to fulfill the container concept)
template <uint8_t t_width>
void swap(int_vector<t_width> & v1, int_vector<t_width> & v2) noexcept
{
    std::swap(v1, v2);
}

template <uint8_t t_width>
void int_vector<t_width>::bit_resize(const size_type size)
{
    if (size > m_capacity || m_data == nullptr) { memory_manager::resize(*this, size); }
    m_size = size;
}

template <uint8_t t_width>
void int_vector<t_width>::bit_resize(const size_type size, const value_type value)
{
    size_type old_size = m_size;
    bit_resize(size);
    auto it = begin() + old_size / m_width;
    util::set_to_value(*this, value, it);
}

template <uint8_t t_width>
auto int_vector<t_width>::get_int(size_type idx, const uint8_t len) const -> value_type
{
#ifdef SDSL_DEBUG
    if (idx + len > m_size)
    {
        throw std::out_of_range("OUT_OF_RANGE_ERROR: int_vector::get_int(size_type, uint8_t); idx+len > size()!");
    }
    if (len > 64) { throw std::out_of_range("OUT_OF_RANGE_ERROR: int_vector::get_int(size_type, uint8_t); len>64!"); }
#endif
    return bits::read_int(m_data + (idx >> 6), idx & 0x3F, len);
}

template <uint8_t t_width>
inline void int_vector<t_width>::set_int(size_type idx, value_type x, const uint8_t len)
{
#ifdef SDSL_DEBUG
    if (idx + len > m_size)
    {
        throw std::out_of_range("OUT_OF_RANGE_ERROR: int_vector::set_int(size_type, uint8_t); idx+len > size()!");
    }
    if (len > 64) { throw std::out_of_range("OUT_OF_RANGE_ERROR: int_vector::set_int(size_type, uint8_t); len>64!"); }
#endif
    bits::write_int(m_data + (idx >> 6), x, idx & 0x3F, len);
}

template <uint8_t t_width>
inline typename int_vector<t_width>::size_type int_vector<t_width>::size() const noexcept
{
    return m_size / m_width;
}

// specialized size method for 64-bit integer vector
template <>
inline typename int_vector<64>::size_type int_vector<64>::size() const noexcept
{
    return m_size >> 6;
}

// specialized size method for 32-bit integer vector
template <>
inline typename int_vector<32>::size_type int_vector<32>::size() const noexcept
{
    return m_size >> 5;
}

// specialized size method for 64-bit integer vector
template <>
inline typename int_vector<16>::size_type int_vector<16>::size() const noexcept
{
    return m_size >> 4;
}

// specialized size method for 64-bit integer vector
template <>
inline typename int_vector<8>::size_type int_vector<8>::size() const noexcept
{
    return m_size >> 3;
}

// specialized size method for bit_vector
template <>
inline typename int_vector<1>::size_type int_vector<1>::size() const noexcept
{
    return m_size;
}

template <uint8_t t_width>
inline typename int_vector<t_width>::size_type int_vector<t_width>::capacity() const noexcept
{
    return m_capacity / m_width;
}

// specialized capacity method for 64-bit integer vector
template <>
inline typename int_vector<64>::size_type int_vector<64>::capacity() const noexcept
{
    return m_capacity >> 6;
}

// specialized capacity method for 32-bit integer vector
template <>
inline typename int_vector<32>::size_type int_vector<32>::capacity() const noexcept
{
    return m_capacity >> 5;
}

// specialized capacity method for 64-bit integer vector
template <>
inline typename int_vector<16>::size_type int_vector<16>::capacity() const noexcept
{
    return m_capacity >> 4;
}

// specialized capacity method for 64-bit integer vector
template <>
inline typename int_vector<8>::size_type int_vector<8>::capacity() const noexcept
{
    return m_capacity >> 3;
}

// specialized capacity method for bit_vector
template <>
inline typename int_vector<1>::size_type int_vector<1>::capacity() const noexcept
{
    return m_capacity;
}

template <uint8_t t_width>
inline auto int_vector<t_width>::operator[](const size_type & idx) noexcept -> reference
{
    assert(idx < this->size());
    size_type i = idx * m_width;
    return reference(this->m_data + (i >> 6), i & 0x3F, m_width);
}

// specialized [] operator for 64 bit access.
template <>
inline auto int_vector<64>::operator[](const size_type & idx) noexcept -> reference
{
    assert(idx < this->size());
    return *(this->m_data + idx);
}

// specialized [] operator for 32 bit access.
template <>
inline auto int_vector<32>::operator[](const size_type & idx) noexcept -> reference
{
    assert(idx < this->size());
    return *(((uint32_t *)(this->m_data)) + idx);
}

// specialized [] operator for 16 bit access.
template <>
inline auto int_vector<16>::operator[](const size_type & idx) noexcept -> reference
{
    assert(idx < this->size());
    return *(((uint16_t *)(this->m_data)) + idx);
}

// specialized [] operator for 8 bit access.
template <>
inline auto int_vector<8>::operator[](const size_type & idx) noexcept -> reference
{
    assert(idx < this->size());
    return *(((uint8_t *)(this->m_data)) + idx);
}

template <uint8_t t_width>
inline auto int_vector<t_width>::operator[](const size_type & idx) const noexcept -> const_reference
{
    assert(idx < this->size());
    return get_int(idx * t_width, t_width);
}

template <>
inline auto int_vector<0>::operator[](const size_type & idx) const noexcept -> const_reference
{
    assert(idx < this->size());
    return get_int(idx * m_width, m_width);
}

template <>
inline auto int_vector<64>::operator[](const size_type & idx) const noexcept -> const_reference
{
    assert(idx < this->size());
    return *(this->m_data + idx);
}

template <>
inline auto int_vector<32>::operator[](const size_type & idx) const noexcept -> const_reference
{
    assert(idx < this->size());
    return *(((uint32_t *)this->m_data) + idx);
}

template <>
inline auto int_vector<16>::operator[](const size_type & idx) const noexcept -> const_reference
{
    assert(idx < this->size());
    return *(((uint16_t *)this->m_data) + idx);
}

template <>
inline auto int_vector<8>::operator[](const size_type & idx) const noexcept -> const_reference
{
    assert(idx < this->size());
    return *(((uint8_t *)this->m_data) + idx);
}

template <>
inline auto int_vector<1>::operator[](const size_type & idx) const noexcept -> const_reference
{
    assert(idx < this->size());
    return ((*(m_data + (idx >> 6))) >> (idx & 0x3F)) & 1;
}

template <uint8_t t_width>
bool int_vector<t_width>::operator<(const int_vector & v) const noexcept
{
    size_type min_size = size();
    if (min_size > v.size()) min_size = v.size();
    for (auto it = begin(), end = begin() + min_size, it_v = v.begin(); it != end; ++it, ++it_v)
    {
        if (*it == *it_v)
            continue;
        else
            return *it < *it_v;
    }
    return size() < v.size();
}

template <uint8_t t_width>
bool int_vector<t_width>::operator>(const int_vector & v) const noexcept
{
    size_type min_size = size();
    if (min_size > v.size()) min_size = v.size();
    for (auto it = begin(), end = begin() + min_size, it_v = v.begin(); it != end; ++it, ++it_v)
    {
        if (*it == *it_v)
            continue;
        else
            return *it > *it_v;
    }
    return size() > v.size();
}

template <uint8_t t_width>
bool int_vector<t_width>::operator<=(const int_vector & v) const noexcept
{
    return *this == v or *this < v;
}

template <uint8_t t_width>
bool int_vector<t_width>::operator>=(const int_vector & v) const noexcept
{
    return *this == v or *this > v;
}

template <uint8_t t_width>
int_vector<t_width> & int_vector<t_width>::operator&=(const int_vector & v)
{
    assert(v.bit_size() == bit_size());
    for (uint64_t i = 0; i < bit_data_size(); ++i) m_data[i] &= v.m_data[i];
    return *this;
}

template <uint8_t t_width>
int_vector<t_width> & int_vector<t_width>::operator|=(const int_vector & v)
{
    assert(bit_size() == v.bit_size());
    for (uint64_t i = 0; i < bit_data_size(); ++i) m_data[i] |= v.m_data[i];
    return *this;
}

template <uint8_t t_width>
int_vector<t_width> & int_vector<t_width>::operator^=(const int_vector & v)
{
    assert(bit_size() == v.bit_size());
    for (uint64_t i = 0; i < bit_data_size(); ++i) m_data[i] ^= v.m_data[i];
    return *this;
}

template <uint8_t t_width>
typename int_vector<t_width>::size_type int_vector<t_width>::write_data(std::ostream & out) const
{
    size_type written_bytes = 0;
    uint64_t * p = m_data;
    size_type idx = 0;
    while (idx + conf::SDSL_BLOCK_SIZE < bit_data_size())
    {
        out.write((char *)p, conf::SDSL_BLOCK_SIZE * sizeof(uint64_t));
        written_bytes += conf::SDSL_BLOCK_SIZE * sizeof(uint64_t);
        p += conf::SDSL_BLOCK_SIZE;
        idx += conf::SDSL_BLOCK_SIZE;
    }
    out.write((char *)p, (bit_data_size() - idx) * sizeof(uint64_t));
    written_bytes += (bit_data_size() - idx) * sizeof(uint64_t);
    return written_bytes;
}

template <uint8_t t_width>
typename int_vector<t_width>::size_type int_vector<t_width>::serialize(std::ostream & out,
                                                                       structure_tree_node * v,
                                                                       std::string name) const
{
    structure_tree_node * child = structure_tree::add_child(v, name, util::class_name(*this));
    size_type written_bytes = int_vector<t_width>::write_header(m_size, m_width, out);
    written_bytes += write_data(out);
    structure_tree::add_size(child, written_bytes);
    return written_bytes;
}

template <uint8_t t_width>
void int_vector<t_width>::load(std::istream & in)
{
    size_type size;
    int_vector<t_width>::read_header(size, m_width, in);

    bit_resize(size);
    uint64_t * p = m_data;
    size_type idx = 0;
    while (idx + conf::SDSL_BLOCK_SIZE < bit_data_size())
    {
        in.read((char *)p, conf::SDSL_BLOCK_SIZE * sizeof(uint64_t));
        p += conf::SDSL_BLOCK_SIZE;
        idx += conf::SDSL_BLOCK_SIZE;
    }
    in.read((char *)p, (bit_data_size() - idx) * sizeof(uint64_t));
}

template <uint8_t t_width>
template <typename archive_t>
inline typename std::enable_if<cereal::traits::is_output_serializable<cereal::BinaryData<int_vector<t_width>>,
                                                                      archive_t>::value,
                               void>::type
int_vector<t_width>::CEREAL_SAVE_FUNCTION_NAME(archive_t & ar) const
{
    ar(CEREAL_NVP(cereal::make_size_tag(static_cast<int_width_type>(m_width))));
    ar(CEREAL_NVP(growth_factor));
    ar(CEREAL_NVP(cereal::make_size_tag(static_cast<size_type>(m_size))));
    ar(cereal::make_nvp("data", cereal::binary_data(m_data, bit_data_size() * sizeof(uint64_t))));
}

template <uint8_t t_width>
template <typename archive_t>
inline typename std::enable_if<!cereal::traits::is_output_serializable<cereal::BinaryData<int_vector<t_width>>,
                                                                       archive_t>::value,
                               void>::type
int_vector<t_width>::CEREAL_SAVE_FUNCTION_NAME(archive_t & ar) const
{
    ar(CEREAL_NVP(m_width));
    ar(CEREAL_NVP(growth_factor));
    ar(CEREAL_NVP(m_size));
    for (value_type const & v : *this) ar(v);
}

template <uint8_t t_width>
template <typename archive_t>
inline typename std::enable_if<cereal::traits::is_input_serializable<cereal::BinaryData<int_vector<t_width>>,
                                                                     archive_t>::value,
                               void>::type
int_vector<t_width>::CEREAL_LOAD_FUNCTION_NAME(archive_t & ar)
{
    ar(CEREAL_NVP(cereal::make_size_tag(m_width)));
    ar(CEREAL_NVP(growth_factor));
    ar(CEREAL_NVP(cereal::make_size_tag(m_size)));
    resize(size());
    ar(cereal::make_nvp("data", cereal::binary_data(m_data, bit_data_size() * sizeof(uint64_t))));
}

template <uint8_t t_width>
template <typename archive_t>
inline typename std::enable_if<!cereal::traits::is_input_serializable<cereal::BinaryData<int_vector<t_width>>,
                                                                      archive_t>::value,
                               void>::type
int_vector<t_width>::CEREAL_LOAD_FUNCTION_NAME(archive_t & ar)
{
    ar(CEREAL_NVP(m_width));
    width(width());
    ar(CEREAL_NVP(growth_factor));
    ar(CEREAL_NVP(m_size));
    resize(size());

    for (size_t i = 0; i < size(); ++i)
    {
        value_type tmp;
        ar(tmp);
        operator[](i) = tmp;
    }
}

} // end namespace sdsl

#include <sdsl/int_vector_buffer.hpp>
#include <sdsl/int_vector_mapper.hpp>

#endif
