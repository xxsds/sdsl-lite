// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
/*!\file util.hpp
 * \brief util.hpp contains some helper methods for int_vector and other stuff like demangle class names.
 * \author Simon Gog
 */
#ifndef INCLUDED_SDSL_UTIL
#define INCLUDED_SDSL_UTIL

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <ctime>      // for rand initialization
#include <functional> // for class_to_hash
#include <iomanip>
#include <iosfwd> // forward declaration of ostream
#include <mutex>
#include <numeric>
#include <random>
#include <sstream>   // for to_string method
#include <stdexcept> // for std::logic_error
#include <stdint.h>  // for uint64_t uint32_t declaration
#include <string.h>  // for strlen and strdup
#include <string>
#include <typeinfo> // for typeid

#include <sdsl/bits.hpp>
#include <sdsl/config.hpp> // for constants
#include <sdsl/ram_fs.hpp>
#include <sdsl/sfstream.hpp>

// macros to transform a defined name to a string
#define SDSL_STR(x) #x
#define SDSL_XSTR(s) SDSL_STR(s)

#include <iomanip>
#include <string>
#include <typeinfo>
#include <vector>

#include <sys/stat.h>  // for file_size
#include <sys/types.h> // for file_size
#include <type_traits>

#ifndef MSVC_COMPILER
#include <cxxabi.h>
#endif

#ifndef _WIN32
#include <libgen.h> // for basename
#include <unistd.h> // for getpid, file_size, clock_gettime

#include <sys/resource.h> // for struct rusage
#include <sys/time.h>     // for struct timeval
#else
#include <iso646.h>
#include <process.h>
#endif

//! Namespace for the succinct data structure library.
namespace sdsl
{

template <uint8_t>
class int_vector; // forward declaration

//! A namespace for helper functions
namespace util
{

//============= Debug information =========================

SDSL_UNUSED static bool verbose = false;

inline void set_verbose()
{
    verbose = true;
}

//============ Manipulating int_vectors ===================

//! Sets all bits of the int_vector to pseudo-random bits.
/*!\param v The int_vector whose bits should be set to random bits
 *  \param seed If seed = 0, the time is used to initialize the
 *              pseudo random number generator, otherwise the seed
 *              parameter is used.
 */
template <class t_int_vec>
void set_random_bits(t_int_vec & v, int seed = 0);
//! Sets all bits of the int_vector to 0-bits.
template <class t_int_vec>
void _set_zero_bits(t_int_vec & v);
//! Sets all bits of the int_vector to 1-bits.
template <class t_int_vec>
void _set_one_bits(t_int_vec & v);

//! Bit compress the int_vector
/*! Determine the biggest value X and then set the
 *  int_width to the smallest possible so that we
 *  still can represent X
 */
template <class t_int_vec>
void bit_compress(t_int_vec & v);

//! Expands the integer width to new_width >= v.width()
template <class t_int_vec>
void expand_width(t_int_vec & v, uint8_t new_width);

//! All elements of v modulo m
template <class t_int_vec>
void mod(t_int_vec & v, typename t_int_vec::size_type m);

inline void cyclic_shifts(uint64_t * vec, uint8_t & n, uint64_t k, uint8_t int_width);

//! Set all entries of int_vector to value k
/*!\param  v The int_vector which should be set
 *  \param  k The value which should be inserted into v.
 *  \par Details
 *   This method pre-calculates the content of at most 64
 *   words and then repeatedly inserts these words into v.
 */
template <class t_int_vec>
void set_to_value(t_int_vec & v, uint64_t k);

//! Set all entries of int_vector starting from iterator it to value k
/*!\param  v The int_vector which should be set
 *  \param  k The value which should be inserted into v.
 *  \param  it The iterator from which on all elements are set to value k.
 *  \par Details
 *   This method pre-calculates the content of at most 64
 *   words and then repeatedly inserts these words into v.
 */
template <class t_int_vec, class t_int_vec_iterator>
void set_to_value(t_int_vec & v, uint64_t k, t_int_vec_iterator it);

//! Sets each entry of the numerical vector v at position \$fi\f$ to value \$fi\$f
template <class t_int_vec>
void set_to_id(t_int_vec & v);

//! Number of set bits in v.
/*!\param v  int_vector object.
 * \return The number of 1-bits in v.
 */
template <class t_int_vec>
typename t_int_vec::size_type cnt_one_bits(const t_int_vec & v);

//! Number of occurrences of bit pattern `10` in v.
/*!\sa getOneBits, getOneZeroBits
 */
template <class t_int_vec>
typename t_int_vec::size_type cnt_onezero_bits(const t_int_vec & v);

//! Number of occurrences of bit pattern `01` in v.
/*!\sa getOneBits, getZeroOneBits
 */
template <class t_int_vec>
typename t_int_vec::size_type cnt_zeroone_bits(const t_int_vec & v);

//! Get the smallest position \f$i\geq idx\f$ where a bit is set
/*!\param v The int_vector in which the bit is searched
 *  \param idx The start position for the search \f$ 0\leq idx < v.bit_size()\f$
 *  \return The smallest position greater or equal to idx, where corresponding bit is 1 or v.bit_size() if no such
 * position exists \par Time complexity \f$ \Order{n} \f$
 */
template <class t_int_vec>
typename t_int_vec::size_type next_bit(const t_int_vec & v, uint64_t idx);

//! Get the greatest position \f$i\leq idx\f$ where a bit is set
/*!\param v The int_vector in which the bit is searched
 *  \param idx The start position for the search \f$ 0\leq idx < v.bit_size()\f$
 *  \return The greatest position smaller or equal to idx, where corresponding bit is 1 or v.bit_size() if no such
 * position exists \par Time complexity \f$ \Order{n} \f$
 */
template <class t_int_vec>
typename t_int_vec::size_type prev_bit(const t_int_vec & v, uint64_t idx);

//============= Handling files =============================

//! Get the size of a file in bytes
/*!\param file  Path to a file.
 *  \returns     Size of the specified file in bytes.
 */
inline size_t file_size(const std::string & file)
{
    if (is_ram_file(file)) { return ram_fs::file_size(file); }
    else
    {
        struct stat fs;
        stat(file.c_str(), &fs);
        return fs.st_size;
    }
}

//! Returns the basename of a file
/*!\param file  Path to a file.
 *  \returns     Basename of the specified file.
 */
inline std::string basename(std::string file)
{
    file = disk_file_name(file); // remove RAM-prefix
#ifdef _WIN32
    char * c = _strdup((const char *)file.c_str());
    char file_name[_MAX_FNAME] = { 0 };
#ifdef MSVC_COMPILER
    ::_splitpath_s(c, NULL, 0, NULL, NULL, file_name, _MAX_FNAME, NULL, 0);
#else
    ::_splitpath(c, NULL, NULL, file_name, NULL);
#endif
    std::string res(file_name);
#else
    char * c = strdup((const char *)file.c_str());
    std::string res = std::string(::basename(c));
#endif
    free(c);
    return res;
}

//! Returns the directory of a file. A trailing `/` will be removed.
/*!\param file  Path to a file.
 *  \returns     Directory name part of the specified path.
 */
inline std::string dirname(std::string file)
{
    bool ram_file = is_ram_file(file);
    file = disk_file_name(file); // remove RAM-prefix
#ifdef _WIN32
    char * c = _strdup((const char *)file.c_str());
    char dir_name[_MAX_DIR] = { 0 };
    char drive[_MAX_DRIVE] = { 0 };
#ifdef MSVC_COMPILER
    ::_splitpath_s(c, drive, _MAX_DRIVE, dir_name, _MAX_DIR, NULL, 0, NULL, 0);
#else
    ::_splitpath(c, drive, dir_name, NULL, NULL);
#endif
    std::string res = std::string(drive) + std::string(dir_name);
#else
    char * c = strdup((const char *)file.c_str());
    std::string res = std::string(::dirname(c));
    auto it = res.begin();
    auto next_it = res.begin() + 1;
    while (it != res.end() and next_it != res.end())
    {
        if (*next_it != '/' or *it != '/') { *(++it) = *next_it; }
        ++next_it;
    }
    res.resize(it - res.begin() + 1);
#endif
    free(c);
    if (ram_file)
    {
        if ("." == res) { res = ram_file_name(""); }
        else if ("/" == res)
        {
            res = ram_file_name(res);
        }
    }
    return res;
}

//! Demangle the class name of typeid(...).name()
/*!
 * \param name A pointer to the result of typeid(...).name()
 */
inline std::string demangle(const std::string & name)
{
#if 1
    char buf[4096];
    size_t size = 4096;
    int status = 0;
    abi::__cxa_demangle(name.c_str(), buf, &size, &status);
    if (status == 0) return std::string(buf);
    return name;
#else
    return name;
#endif
}

//! Demangle the class name of typeid(...).name() and remove the "sdsl::"-prefix, "unsigned int",...
inline std::string demangle2(const std::string & name)
{
    std::string result = demangle(name);
    std::vector<std::string> words_to_delete;
    words_to_delete.push_back("sdsl::");
    words_to_delete.push_back("(unsigned char)");
    words_to_delete.push_back(", unsigned long");

    for (size_t k = 0; k < words_to_delete.size(); ++k)
    {
        std::string w = words_to_delete[k];
        for (size_t i = result.find(w); i != std::string::npos; i = result.find(w, i))
        {
            result.erase(i, w.length());
            ++i;
        }
    }
    size_t index = 0;
    std::string to_replace = "int_vector<1>";
    while ((index = result.find(to_replace, index)) != std::string::npos)
    {
        result.replace(index, to_replace.size(), "bit_vector");
    }
    return result;
}

//! Convert type to string
template <typename T>
std::string to_string(const T & t, int w);

//! Transforms the demangled class name of an object to a hash value.
template <class T>
uint64_t hashvalue_of_classname(const T &)
{
    std::hash<std::string> str_hash;
    return str_hash(sdsl::util::demangle2(typeid(T).name()));
}

//! Transforms the demangled class name of an object to a hash value.
template <class T>
std::string class_to_hash(const T & t)
{
    return to_string(hashvalue_of_classname(t));
}

template <class T>
std::string class_name(const T & t)
{
    std::string result = demangle2(typeid(t).name());
    size_t template_pos = result.find("<");
    if (template_pos != std::string::npos) { result = result.erase(template_pos); }
    return result;
}

// convert an errno number to a readable msg
inline char * str_from_errno()
{
#ifdef MSVC_COMPILER
#pragma warning(disable : 4996)
    return strerror(errno);
#pragma warning(default : 4996)
#else
    return strerror(errno);
#endif
}

struct _id_helper_struct
{
    uint64_t id = 0;
};

extern inline uint64_t _id_helper()
{
    static _id_helper_struct data;
    return data.id++;
}

//! Get the process id of the current process
inline uint64_t pid()
{
#ifdef MSVC_COMPILER
    return _getpid();
#else
    return getpid();
#endif
}

//! Get a unique id inside the process
inline uint64_t id()
{
    return _id_helper();
}

template <typename T>
std::string to_latex_string(const T & t);

inline std::string to_latex_string(unsigned char c)
{
    if (c == '_')
        return "\\_";
    else if (c == '\0')
        return "\\$";
    else
        return to_string(c);
}

//! Delete all files of the file_map.
inline void delete_all_files(tMSS & file_map)
{
    for (auto file_pair : file_map) { sdsl::remove(file_pair.second); }
    file_map.clear();
}

//! clear the space used by x
/*!
 * \param x Reference to the data structure.
 */
template <class T>
void clear(T & x)
{
    T y;
    x = std::move(y);
}

//! Swap support data structure and assign to new vector
/*!\param s1 First support structure.
 *  \param s2 Second support structure.
 *  \param p1 First supported structure.
 *  \param p2 Second supported structure.
 *  s1 is swapped with s2 and after the execution s1 supports p1 and s2 supports
 *  p2. I.e. if p1 and p2 are members of a complex data structure, we have to
 *  swap p1 and p2 before we use this method.
 */
template <class S, class P>
void swap_support(S & s1, S & s2, const P * p1, const P * p2)
{
    std::swap(s1, s2);
    s1.set_vector(p1);
    s2.set_vector(p2);
}

//! Initialise support data structure with
/*!\param s Support structure which should be initialized
 *  \param x Pointer to the data structure which should be supported.
 */
template <class S, class X>
void init_support(S & s, const X * x)
{
    S temp(x);           // generate a temporary support object
    s = std::move(temp); // swap its content with the target object
    s.set_vector(x);     // set the support object's  pointer to x
}

//! Create 2^{log_s} random integers mod m with seed x
/*
 */
template <class t_int_vec>
t_int_vec rnd_positions(uint8_t log_s, uint64_t & mask, uint64_t mod = 0, uint64_t seed = 17)
{
    mask = (1 << log_s) - 1;
    t_int_vec rands(1 << log_s, 0);
    set_random_bits(rands, seed);
    if (mod > 0) { util::mod(rands, mod); }
    return rands;
}

//! Checks at compile time whether type is regular or not
/*  static_assert(is_regular<YOUR_TYPE>::value);
 *  Code is from a talk of Aerix Consulting
 */
template <typename T>
struct is_regular
  : std::integral_constant<bool,
                           std::is_default_constructible<T>::value && std::is_copy_constructible<T>::value &&
                                                                             std::is_move_constructible<T>::value &&
                                                                             std::is_copy_assignable<T>::value &&
                                                                             std::is_move_assignable<T>::value>
{};

} // end namespace util

//==================== Template functions ====================

template <class t_int_vec>
void util::set_random_bits(t_int_vec & v, int seed)
{
    std::mt19937_64 rng;
    if (0 == seed) { rng.seed(std::chrono::system_clock::now().time_since_epoch().count() + util::id()); }
    else
        rng.seed(seed);

    uint64_t * data = v.data();
    if (v.empty()) return;
    *data = rng();
    for (typename t_int_vec::size_type i = 1; i < ((v.bit_size() + 63) >> 6); ++i) { *(++data) = rng(); }
}

// all elements of vector v modulo m
template <class t_int_vec>
void util::mod(t_int_vec & v, typename t_int_vec::size_type m)
{
    for (typename t_int_vec::size_type i = 0; i < v.size(); ++i) { v[i] = v[i] % m; }
}

template <class t_int_vec>
void util::bit_compress(t_int_vec & v)
{
    auto max_elem = std::max_element(v.begin(), v.end());
    uint64_t max = 0;
    if (max_elem != v.end()) { max = *max_elem; }
    uint8_t min_width = bits::hi(max) + 1;
    uint8_t old_width = v.width();
    if (old_width > min_width)
    {
        const uint64_t * read_data = v.data();
        uint64_t * write_data = v.data();
        uint8_t read_offset = 0;
        uint8_t write_offset = 0;
        for (typename t_int_vec::size_type i = 0; i < v.size(); ++i)
        {
            uint64_t x = bits::read_int_and_move(read_data, read_offset, old_width);
            bits::write_int_and_move(write_data, x, write_offset, min_width);
        }
        v.bit_resize(v.size() * min_width);
        v.width(min_width);
        // v.shrink_to_fit(); TODO(cpockrandt): comment in once int_vector_mapper has the same interface
    }
}

template <class t_int_vec>
void util::expand_width(t_int_vec & v, uint8_t new_width)
{
    uint8_t old_width = v.width();
    typename t_int_vec::size_type n = v.size();
    if (new_width > old_width)
    {
        if (n > 0)
        {
            typename t_int_vec::size_type i, old_pos, new_pos;
            new_pos = (n - 1) * new_width;
            old_pos = (n - 1) * old_width;
            v.bit_resize(v.size() * new_width);
            for (i = 0; i < n; ++i, new_pos -= new_width, old_pos -= old_width)
            {
                v.set_int(new_pos, v.get_int(old_pos, old_width), new_width);
            }
        }
        v.width(new_width);
    }
}

template <class t_int_vec>
void util::_set_zero_bits(t_int_vec & v)
{
    std::for_each(v.data(), v.data() + ((v.bit_size() + 63) >> 6), [](uint64_t & value) { value = 0ULL; });
}

template <class t_int_vec>
void util::_set_one_bits(t_int_vec & v)
{
    std::for_each(v.data(), v.data() + ((v.bit_size() + 63) >> 6), [](uint64_t & value) { value = -1ULL; });
}

inline void util::cyclic_shifts(uint64_t * vec, uint8_t & n, uint64_t k, uint8_t int_width)
{
    n = 0;
    vec[0] = 0;
    uint8_t offset = 0;
    k &= 0xFFFFFFFFFFFFFFFFULL >> (64 - int_width);
    do { // loop terminates after at most 64 iterations
        vec[n] |= k << offset;
        offset += int_width;
        if (offset >= 64)
        {
            ++n;
            if (int_width == 64) return;
            assert(int_width - (offset - 64) < 64);
            vec[n] = k >> (int_width - (offset - 64));
            offset -= 64;
        }
    } while (offset != 0);
}

template <class t_int_vec>
void util::set_to_value(t_int_vec & v, uint64_t k)
{
    uint64_t * data = v.data();
    if (v.empty()) return;
    uint8_t int_width = v.width();
    if (int_width == 0) { throw std::logic_error("util::set_to_value can not be performed with int_width=0!"); }
    if (0 == k)
    {
        _set_zero_bits(v);
        return;
    }
    if (bits::lo_set[int_width] == k)
    {
        _set_one_bits(v);
        return;
    }
    uint8_t n;
    uint64_t vec[65];
    util::cyclic_shifts(vec, n, k, int_width);

    typename t_int_vec::size_type n64 = (v.bit_size() + 63) >> 6;
    for (typename t_int_vec::size_type i = 0; i < n64;)
    {
        for (uint64_t ii = 0; ii < n and i < n64; ++ii, ++i) { *(data++) = vec[ii]; }
    }
}

template <class t_int_vec, class t_int_vec_iterator>
void util::set_to_value(t_int_vec & v, uint64_t k, t_int_vec_iterator it)
{
    typedef typename t_int_vec::size_type size_type;

    if (v.empty()) return;
    uint8_t int_width = v.width();
    if (int_width == 0) { throw std::logic_error("util::set_to_value can not be performed with int_width=0!"); }
    uint8_t n;
    uint64_t vec[65];
    util::cyclic_shifts(vec, n, k, int_width);

    size_type words = (v.bit_size() + 63) >> 6;
    size_type word_pos = ((it - v.begin()) * int_width) >> 6;
    uint8_t pos_in_word = ((it - v.begin()) * int_width) - (word_pos << 6); // ((it - v.begin()) * int_width) % 64
    uint8_t cyclic_shift = word_pos % n;

    uint64_t * data = v.data() + word_pos;
    *(data) &= bits::lo_set[pos_in_word];                         // unset first bits
    *(data) |= bits::lo_unset[pos_in_word] & vec[cyclic_shift++]; // set last bits
    ++word_pos;

    while (word_pos < words)
    {
        for (; cyclic_shift < n && word_pos < words; ++cyclic_shift, ++word_pos) { *(++data) = vec[cyclic_shift]; }
        cyclic_shift = 0;
    }
}

//! Set v[i] = i for i=[0..v.size()-1]
template <class t_int_vec>
void util::set_to_id(t_int_vec & v)
{
    std::iota(v.begin(), v.end(), 0ULL);
}

template <class t_int_vec>
typename t_int_vec::size_type util::cnt_one_bits(const t_int_vec & v)
{
    const uint64_t * data = v.data();
    if (v.empty()) return 0;
    typename t_int_vec::size_type result = bits::cnt(*data);
    for (typename t_int_vec::size_type i = 1; i < ((v.bit_size() + 63) >> 6); ++i) { result += bits::cnt(*(++data)); }
    if (v.bit_size() & 0x3F) { result -= bits::cnt((*data) & (~bits::lo_set[v.bit_size() & 0x3F])); }
    return result;
}

template <class t_int_vec>
typename t_int_vec::size_type util::cnt_onezero_bits(const t_int_vec & v)
{
    const uint64_t * data = v.data();
    if (v.empty()) return 0;
    uint64_t carry = 0, oldcarry = 0;
    typename t_int_vec::size_type result = bits::cnt10(*data, carry);
    for (typename t_int_vec::size_type i = 1; i < ((v.bit_size() + 63) >> 6); ++i)
    {
        oldcarry = carry;
        result += bits::cnt10(*(++data), carry);
    }
    if (v.bit_size() & 0x3F)
    { // if bit_size is not a multiple of 64, subtract the counts of the additional bits
        result -= bits::cnt(bits::map10(*data, oldcarry) & bits::lo_unset[v.bit_size() & 0x3F]);
    }
    return result;
}

template <class t_int_vec>
typename t_int_vec::size_type util::cnt_zeroone_bits(const t_int_vec & v)
{
    const uint64_t * data = v.data();
    if (v.empty()) return 0;
    uint64_t carry = 1, oldcarry = 1;
    typename t_int_vec::size_type result = bits::cnt01(*data, carry);
    for (typename t_int_vec::size_type i = 1; i < ((v.bit_size() + 63) >> 6); ++i)
    {
        oldcarry = carry;
        result += bits::cnt01(*(++data), carry);
    }
    if (v.bit_size() & 0x3F)
    { // if bit_size is not a multiple of 64, subtract the counts of the additional bits
        result -= bits::cnt(bits::map01(*data, oldcarry) & bits::lo_unset[v.bit_size() & 0x3F]);
    }
    return result;
}

template <class t_int_vec>
typename t_int_vec::size_type util::next_bit(const t_int_vec & v, uint64_t idx)
{
    uint64_t pos = idx >> 6;
    uint64_t node = v.data()[pos];
    node >>= (idx & 0x3F);
    if (node) { return idx + bits::lo(node); }
    else
    {
        ++pos;
        while ((pos << 6) < v.bit_size())
        {
            if (v.data()[pos]) { return (pos << 6) | bits::lo(v.data()[pos]); }
            ++pos;
        }
        return v.bit_size();
    }
}

template <class t_int_vec>
typename t_int_vec::size_type util::prev_bit(const t_int_vec & v, uint64_t idx)
{
    uint64_t pos = idx >> 6;
    uint64_t node = v.data()[pos];
    node <<= 63 - (idx & 0x3F);
    if (node) { return bits::hi(node) + (pos << 6) - (63 - (idx & 0x3F)); }
    else
    {
        --pos;
        while ((pos << 6) < v.bit_size())
        {
            if (v.data()[pos]) { return (pos << 6) | bits::hi(v.data()[pos]); }
            --pos;
        }
        return v.bit_size();
    }
}

template <typename T>
std::string util::to_string(const T & t, int w)
{
    std::stringstream ss;
    ss << std::setw(w) << t;
    return ss.str();
}

template <typename T>
std::string util::to_latex_string(const T & t)
{
    return to_string(t);
}

} // end namespace sdsl
#endif
