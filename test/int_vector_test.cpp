#include "sdsl/int_vector.hpp"
#include "sdsl/util.hpp"
#include "gtest/gtest.h"
#include <vector>
#include <string>
#include <random>
#include <algorithm>
#include <chrono>

namespace
{

typedef sdsl::int_vector<>::size_type size_type;
typedef sdsl::int_vector<>::value_type value_type;

std::string temp_dir;

// The fixture for testing class int_vector.
class int_vector_test : public ::testing::Test
{
    protected:

        int_vector_test() {}

        virtual ~int_vector_test() {}

        virtual void SetUp()
        {
            std::mt19937_64 rng;
            {
                std::uniform_int_distribution<uint64_t> distribution(0, 100000);
                auto dice = bind(distribution, rng);
                for (size_type i=0; i<128; ++i) {
                    vec_sizes.push_back(dice());
                }
            }
            {
                std::uniform_int_distribution<uint64_t> distribution(0, 10000000);
                auto dice = bind(distribution, rng);
                for (size_type i=0; i < 10; ++i) {
                    vec_sizes.push_back(dice());
                }
            }
        }

        virtual void TearDown() {}

        std::vector<size_type> vec_sizes = {0,64,65,127,128}; // different sizes for the vectors
};

template<class t_iv>
void test_Constructors(uint8_t template_width, size_type constructor_size, uint8_t constructor_width)
{
    typedef typename t_iv::value_type value_type;
    static_assert(sdsl::util::is_regular<t_iv>::value, "Type is not regular");
    std::mt19937_64 rng;
    {
        // Constructor without argument
        t_iv iv;
        ASSERT_EQ((size_type)0, iv.size()); // default size should be 0
        ASSERT_EQ((uint8_t)template_width, iv.width()); // verify default width of each element
    }
    {
        // Constructor with one argument
        t_iv iv(constructor_size);
        ASSERT_EQ(constructor_size, iv.size());
        ASSERT_EQ(template_width, iv.width());
        for (size_type j=0; j < iv.size(); ++j) { // should be initialized with 0s
            ASSERT_EQ((typename t_iv::value_type)0, (typename t_iv::value_type)iv[j]);
        }
    }
    {
        // Constructor with two arguments
        value_type expected_val = rng();
        t_iv iv(constructor_size, expected_val);
        ASSERT_EQ(constructor_size, iv.size());
        ASSERT_EQ(template_width, iv.width());
        expected_val &= sdsl::bits::lo_set[iv.width()];
        for (size_type j=0; j < iv.size(); ++j) { // should be initialized with expected_val
            ASSERT_EQ(expected_val, iv[j]);
        }
    }
    {
        // Constructor with three arguments
        value_type expected_val = rng();
        t_iv iv(constructor_size, expected_val, constructor_width);
        ASSERT_EQ(constructor_size, iv.size());
        if (iv.fixed_int_width == 0) {
            ASSERT_EQ(constructor_width, iv.width());
        } else {
            ASSERT_EQ(template_width, iv.width());
        }
        expected_val &= sdsl::bits::lo_set[iv.width()];
        for (size_type j=0; j < iv.size(); ++j) { // should be initialized with expected_val
            ASSERT_EQ(expected_val, iv[j]);
        }
    }
    {
        // Constructor with initalizer list (ignores constructor_size)
        t_iv iv({1, 0, 1});
        ASSERT_EQ(iv.size(), (size_type)3);
        ASSERT_EQ(iv[0], (value_type)1);
        ASSERT_EQ(iv[1], (value_type)0);
        ASSERT_EQ(iv[2], (value_type)1);
    }
    {
        // Constructor with iterator pair
        t_iv iv(constructor_size);
        sdsl::util::set_to_id(iv);
        t_iv iv2(iv.begin() + constructor_size/4, iv.end() - constructor_size/4); // copy some infix
        ASSERT_EQ(iv2.size(), constructor_size - 2*(constructor_size/4));
        for (auto it = iv2.begin(); it != iv2.end(); ++it)
            ASSERT_EQ(*it, iv[it - iv2.begin() + constructor_size/4]);
    }
}



//! Test Constructors
TEST_F(int_vector_test, constructors)
{
    for (auto size : vec_sizes) {
        if (size<1000) {                                // Test only for short sizes,
            for (uint8_t width=1; width<=64; ++width) { // but for all possible widths
                // unspecialized
                test_Constructors<sdsl::int_vector<>   >(64, size, width);
                test_Constructors<sdsl::int_vector<3>  >(3, size, width);
                test_Constructors<sdsl::int_vector<31> >(31, size, width);
                // specialized
                test_Constructors<sdsl::bit_vector     >(1, size, width);
                test_Constructors<sdsl::int_vector<8>  >(8, size, width);
                test_Constructors<sdsl::int_vector<16> >(16, size, width);
                test_Constructors<sdsl::int_vector<32> >(32, size, width);
                test_Constructors<sdsl::int_vector<64> >(64, size, width);
            }
        }
    }
}

TEST_F(int_vector_test, width)
{
    {
        size_type len = 1000;
        sdsl::int_vector<> v(len, 0xF0, 8);
        ASSERT_EQ(len, v.size());
        ASSERT_EQ((uint8_t)8, v.width());
        v.width(4);
        ASSERT_EQ((uint8_t)4, v.width());
        ASSERT_EQ(2*len, v.size());
        for (size_type i=0; i<v.size()/2; i+=2) {
            ASSERT_EQ(0x0U, v[i*2]);
            ASSERT_EQ(0xFU, v[i*2+1]);
        }
    }
    {
        sdsl::int_vector<16> v1(1000);
        sdsl::int_vector<32> v2(1000);
        sdsl::util::set_to_id(v1);
        sdsl::util::set_to_id(v2);
        ASSERT_TRUE(v1 == v2);
        ASSERT_FALSE(v1 != v2);
    }
    {
        sdsl::int_vector< 8> v1(1000);
        sdsl::int_vector<16> v2(1000);
        sdsl::util::set_to_id(v1);
        sdsl::util::set_to_id(v2);
        ASSERT_FALSE(v1 == v2);
        ASSERT_TRUE(v1 != v2);
    }
}

TEST_F(int_vector_test, swap)
{
    std::mt19937_64 rng;
    for (size_type i=0; i < vec_sizes.size(); ++i) {
        const size_type val = rng();
        sdsl::int_vector<> iv(vec_sizes[i], val);
        {
            sdsl::int_vector<> tmp;
            ASSERT_EQ((size_type)0, tmp.size());
            std::swap(tmp,iv);
            ASSERT_EQ((size_type)0, iv.size());
            ASSERT_EQ(vec_sizes[i], tmp.size());
            for (size_type j=0; j < tmp.size(); ++j) {
                ASSERT_EQ(val, tmp[j]);
            }
        }
    }
}

TEST_F(int_vector_test, access)
{
    std::mt19937_64 rng;
    sdsl::int_vector<3> iv(10, 0);
    std::generate(iv.begin(), iv.end(), [&rng] () { return rng(); });

    // front()
    ASSERT_EQ(iv.front(), iv[0]);
    // back()
    ASSERT_EQ(iv.back(), iv[iv.size() - 1]);
    // at()
    for (size_type j=0; j < iv.size(); ++j) {
        ASSERT_EQ(iv.at(j), iv[j]);
    }
}

template<class t_iv>
void test_AssignAndModifyElement(uint64_t size, uint8_t width)
{
    std::mt19937_64 rng;
    t_iv iv(size, 0, width);
    for (size_type i=1; i<iv.size(); ++i) {
        value_type exp_v = rng(), tmp = rng();

        // Assign Test
        iv[i] = exp_v;
        ASSERT_EQ(exp_v & sdsl::bits::lo_set[width], iv[i]);

        // Modify Test
        iv[i] += tmp;
        exp_v += tmp;
        ASSERT_EQ(exp_v & sdsl::bits::lo_set[width], iv[i]);
        iv[i] += -1;
        exp_v += -1;
        ASSERT_EQ(exp_v & sdsl::bits::lo_set[width], iv[i]);
        iv[i] -= tmp;
        exp_v -= tmp;
        ASSERT_EQ(exp_v & sdsl::bits::lo_set[width], iv[i]);
        iv[i] -= -1;
        exp_v -= -1;
        ASSERT_EQ(exp_v & sdsl::bits::lo_set[width], iv[i]);
        ASSERT_EQ(exp_v & sdsl::bits::lo_set[width], iv[i]++);
        exp_v++;
        ASSERT_EQ(exp_v & sdsl::bits::lo_set[width], iv[i]);
        ASSERT_EQ(exp_v & sdsl::bits::lo_set[width], iv[i]--);
        exp_v--;
        ASSERT_EQ(exp_v & sdsl::bits::lo_set[width], iv[i]);
        ++exp_v;
        ASSERT_EQ(exp_v & sdsl::bits::lo_set[width], ++iv[i]);
        ASSERT_EQ(exp_v & sdsl::bits::lo_set[width], iv[i]);
        --exp_v;
        ASSERT_EQ(exp_v & sdsl::bits::lo_set[width], --iv[i]);
        ASSERT_EQ(exp_v & sdsl::bits::lo_set[width], iv[i]);

        // Compare Test
        iv[i] = exp_v;
        iv[i-1] = tmp;
        exp_v &= sdsl::bits::lo_set[width];
        tmp &= sdsl::bits::lo_set[width];
        ASSERT_EQ((exp_v!=tmp), (iv[i]!=iv[i-1]));
        ASSERT_EQ((exp_v==tmp), (iv[i]==iv[i-1]));
        ASSERT_EQ((exp_v<=tmp), (iv[i]<=iv[i-1]));
        ASSERT_EQ((exp_v< tmp), (iv[i]<iv[i-1]));
        ASSERT_EQ((exp_v>=tmp), (iv[i]>=iv[i-1]));
        ASSERT_EQ((exp_v> tmp), (iv[i]>iv[i-1]));
        iv[i-1] = exp_v;
        ASSERT_FALSE(iv[i]!=iv[i-1]);
        ASSERT_TRUE(iv[i]==iv[i-1]);
    }
}

template<>
void test_AssignAndModifyElement<sdsl::bit_vector>(uint64_t size, uint8_t width)
{
    std::mt19937_64 rng(13);
    std::uniform_int_distribution<uint64_t> distribution(0, 9);

    sdsl::bit_vector bv(size, 0, width);
    for (size_type i=0; i<bv.size(); ++i) {
        value_type exp_v = distribution(rng);
        bv[i] = exp_v;
        ASSERT_EQ((bool)exp_v, bv[i]);
    }
    bv.flip();

    rng.seed(13); // To get the same values
    for (size_type i=0; i<bv.size(); ++i) {
        value_type exp_v = !distribution(rng);
        ASSERT_EQ((bool)exp_v, bv[i]);
    }
}

TEST_F(int_vector_test, AssignAndModifyElement)
{
    // unspecialized vector for each possible width
    for (uint8_t width=1; width <= 64; ++width) {
        test_AssignAndModifyElement< sdsl::int_vector<> >(100000, width);
    }
    // specialized vectors
    test_AssignAndModifyElement<sdsl::bit_vector     >(100000,  1);
    test_AssignAndModifyElement<sdsl::int_vector< 8> >(100000,  8);
    test_AssignAndModifyElement<sdsl::int_vector<16> >(100000, 16);
    test_AssignAndModifyElement<sdsl::int_vector<32> >(100000, 32);
    test_AssignAndModifyElement<sdsl::int_vector<64> >(100000, 64);
}

template<class t_iv>
void test_AssignAndResize(uint64_t size, uint8_t width)
{
    typedef typename t_iv::value_type value_type;
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();

    std::mt19937_64 rng(ns);
    {
        // assign(size, value)
        t_iv iv(size, rng(), width);
        value_type expected_val = rng() & sdsl::bits::lo_set[iv.width()];
        uint64_t new_size = std::max((size_type)0, size + (rng() % 100) - 50); // increase or decrease randomly
        iv.assign(new_size, expected_val);
        ASSERT_EQ(iv.size(), new_size);
        for (size_type j=0; j < iv.size(); ++j)
            ASSERT_EQ(expected_val, iv[j]);
    }
    {
        // assign(initializer_list)
        t_iv iv(size, rng(), width);
        iv.assign({1, 0, 1});
        ASSERT_EQ(iv.size(), (size_type)3);
        ASSERT_EQ(iv[0], (value_type)1);
        ASSERT_EQ(iv[1], (value_type)0);
        ASSERT_EQ(iv[2], (value_type)1);
    }
    {
        // assign(first, last)
        t_iv iv(size, 0, width), iv2(size, 0, width);
        sdsl::util::set_to_id(iv);
        iv2.assign(iv.begin() + size/4, iv.end() - size/4); // copy some infix
        ASSERT_EQ(iv2.size(), size - 2*(size/4));
        for (auto it = iv2.begin(); it != iv2.end(); ++it)
            ASSERT_EQ(*it, iv[it - iv2.begin() + size/4]);
    }
    {
        // resize(size)
        value_type val = rng() & sdsl::bits::lo_set[width];
        t_iv iv(size, val, width);
        uint64_t new_size = std::max((size_type)0, size + (rng() % 100) - 50); // increase or decrease randomly
        iv.resize(new_size);
        ASSERT_EQ(iv.size(), new_size);
        for (size_type j=0; j < std::min(size, new_size); ++j)
            ASSERT_EQ(val, iv[j]); // old values
        for (size_type j=size; j < new_size; ++j)
            ASSERT_EQ((value_type)0, iv[j]); // new values
    }
    {
        // resize(size, v)
        size = 50;
        value_type val1 = rng() & sdsl::bits::lo_set[width];
        value_type val2 = rng() & sdsl::bits::lo_set[width];
        t_iv iv(size, val1, width);
        uint64_t new_size = std::max((size_type)0, size + (rng() % 100) - 50); // increase or decrease randomly
        iv.resize(new_size, val2);
        ASSERT_EQ(iv.size(), new_size);
        for (size_type j=0; j < std::min(size, new_size); ++j)
            ASSERT_EQ(val1, iv[j]); // old values
        for (size_type j=size; j < new_size; ++j)
            ASSERT_EQ(val2, iv[j]); // new values
    }
    {
        // clear()
        t_iv iv(size, rng(), width);
        size_type old_bit_capacity = iv.bit_capacity();
        iv.clear();
        ASSERT_EQ(iv.size(), (size_type)0);
        ASSERT_EQ(iv.bit_capacity(), old_bit_capacity);
    }
    {
        // reserve / resize / shrink behavior
        t_iv iv(size, 0, width);
        size_type initial_capacity = iv.capacity();
        ASSERT_EQ(iv.bit_capacity(), (iv.bit_capacity() >> 6) << 6); // bit_capacity must always be multiple of 64

        iv.reserve(initial_capacity + 100);
        ASSERT_TRUE(iv.capacity() >= initial_capacity + 100);
        ASSERT_EQ(iv.bit_capacity(), (iv.bit_capacity() >> 6) << 6); // bit_capacity must always be multiple of 64

        iv.shrink_to_fit();
        ASSERT_TRUE((((size * width + 63) >> 6) << 6) <= iv.bit_capacity());
    }
}

TEST_F(int_vector_test, AssignAndResize)
{
    // unspecialized vector for each possible width
    for (uint8_t width=1; width <= 64; ++width) {
        test_AssignAndResize<sdsl::int_vector<> >(100000, width);
    }
    // specialized vectors
    test_AssignAndResize<sdsl::bit_vector     >(100000,  1);
    test_AssignAndResize<sdsl::int_vector< 8> >(100000,  8);
    test_AssignAndResize<sdsl::int_vector<16> >(100000, 16);
    test_AssignAndResize<sdsl::int_vector<32> >(100000, 32);
    test_AssignAndResize<sdsl::int_vector<64> >(100000, 64);
}

template<class t_iv>
void test_InsertAndDelete(uint64_t size, uint8_t width)
{
    typedef typename t_iv::value_type value_type;
    std::mt19937_64 rng;
    value_type val1 = rng() & sdsl::bits::lo_set[width];
    value_type val2 = rng() & sdsl::bits::lo_set[width];
    {
        // insert(it, value)
        t_iv iv(size, val1, width);
        iv.insert(iv.begin(), val2); // insert at beginning
        iv.insert(iv.end(), val2); // insert at end
        iv.insert(iv.begin() + 2, val2); // insert in the middle
        ASSERT_EQ(iv.size(), size + 3);
        for (size_type j=0; j < iv.size(); ++j)
        {
            if (j == 0 || j == 2 || j == iv.size() - 1)
                ASSERT_EQ(val2, iv[j]);
            else
                ASSERT_EQ(val1, iv[j]);
        }
        // erase(it)
        iv.erase(iv.begin() + 2);
        iv.erase(iv.begin());
        iv.erase(iv.end() - 1);
        ASSERT_EQ(iv.size(), size);
        for (size_type j=0; j < iv.size(); ++j)
            ASSERT_EQ(val1, iv[j]);
    }
    {
        // insert(it, n, value)
        t_iv iv(size, val1, width);
        iv.insert(iv.begin(), 3, val2); // insert 3 bits at beginning
        iv.insert(iv.end(), 0, val2); // insert 0 bits at end
        iv.insert(iv.begin() + 4, 2, val2); // insert 2 bits in the middle
        ASSERT_EQ(iv.size(), size + 5);
        for (size_type j=0; j < iv.size(); ++j)
        {
            if (j <= 2 || j == 4 || j == 5)
                ASSERT_EQ(val2, iv[j]);
            else
                ASSERT_EQ(val1, iv[j]);
        }
    }
    {
        // insert(it, first, last)
        t_iv iv(size, val1, width);
        t_iv iv2(2, 1, width); // 1 1
        iv.insert(iv.begin(), iv2.begin(), iv2.end()); // insert "1 1" at beginning
        iv.insert(iv.end(), iv2.begin(), iv2.end()); // insert "1 1" at end
        iv.insert(iv.begin() + 3, iv2.begin(), iv2.end() - 1); // insert "1" in the middle
        ASSERT_EQ(iv.size(), size + 5);
        for (size_type j=0; j < iv.size(); ++j)
        {
            if (j < 2 || j == 3 || j > iv.size() - 3)
                ASSERT_EQ((value_type)1, iv[j]);
            else
                ASSERT_EQ(val1, iv[j]);
        }
        // erase(first, last)
        iv.erase(iv.begin() + 3, iv.begin() + 4); // remove from middle
        iv.erase(iv.begin(), iv.begin() + 2); // remove from beginning
        iv.erase(iv.end() - 2, iv.end()); // remove from end
        ASSERT_EQ(iv.size(), size);
        for (size_type j=0; j < iv.size(); ++j)
            ASSERT_EQ(val1, iv[j]);
    }
    {
        // insert(it, initializer_list)
        t_iv iv(size, val1, width);
        iv.insert(iv.begin(), {});
        iv.insert(iv.begin(), {val2, val2});
        ASSERT_EQ(iv.size(), size + 2);
        for (size_type j=0; j < iv.size(); ++j)
        {
            if (j < 2)
                ASSERT_EQ(val2, iv[j]);
            else
                ASSERT_EQ(val1, iv[j]);
        }
    }
    {
        // push_back(value)
        t_iv iv(size, val1, width);
        iv.push_back(val2);
        ASSERT_EQ(iv.size(), size + 1);
        for (size_type j=0; j < iv.size(); ++j)
        {
            if (j == iv.size() - 1)
                ASSERT_EQ(val2, iv[j]);
            else
                ASSERT_EQ(val1, iv[j]);
        }
        // pop_back()
        iv.pop_back();
        ASSERT_EQ(iv.size(), size);
        for (size_type j=0; j < iv.size(); ++j)
        {
            ASSERT_EQ(val1, iv[j]);
        }
    }
}

TEST_F(int_vector_test, InsertAndDelete)
{
    // unspecialized vector for each possible width
    for (uint8_t width=1; width <= 64; ++width) {
        test_InsertAndDelete<sdsl::int_vector<> >(100000, width);
    }
    // specialized vectors
    test_InsertAndDelete<sdsl::bit_vector     >(100000,  1);
    test_InsertAndDelete<sdsl::int_vector< 8> >(100000,  8);
    test_InsertAndDelete<sdsl::int_vector<16> >(100000, 16);
    test_InsertAndDelete<sdsl::int_vector<32> >(100000, 32);
    test_InsertAndDelete<sdsl::int_vector<64> >(100000, 64);
}

TEST_F(int_vector_test, stl)
{
    for (size_type i=0; i < vec_sizes.size(); ++i) {
        sdsl::int_vector<> iv(vec_sizes[i]);
        ASSERT_EQ(vec_sizes[i], iv.size());
        auto cnt = iv.size();
        for (auto x : iv) {
            x = --cnt;
        }
        std::sort(iv.begin(), iv.end());
        sdsl::int_vector<>::value_type last = 0;
        for (const auto& x : iv) {
            ASSERT_TRUE(x >= last);
            last = x;
        }
    }
}

template<class t_iv>
void test_SerializeAndLoad(uint8_t width=1)
{
    std::mt19937_64 rng;
    t_iv iv(sdsl::conf::SDSL_BLOCK_SIZE+1000000, 0, width);
    for (size_type i=0; i<iv.size(); ++i)
        iv[i] = rng();
    std::string file_name = temp_dir+"/int_vector";
    sdsl::store_to_file(iv, file_name);
    t_iv iv2;
    sdsl::load_from_file(iv2, file_name);
    ASSERT_EQ(iv.size(), iv2.size());
    ASSERT_EQ(iv.width(), iv2.width());
    for (size_type i=0; i<iv.size(); ++i)
        ASSERT_EQ(iv[i], iv2[i]);
    sdsl::remove(file_name);
}

TEST_F(int_vector_test, serialize_and_load)
{
    // unspecialized vector for each possible width
    for (uint8_t width=1; width <= 64; ++width) {
        test_SerializeAndLoad< sdsl::int_vector<> >(width);
    }
    // specialized vectors
    test_SerializeAndLoad<sdsl::bit_vector     >();
    test_SerializeAndLoad<sdsl::int_vector< 8> >();
    test_SerializeAndLoad<sdsl::int_vector<16> >();
    test_SerializeAndLoad<sdsl::int_vector<32> >();
    test_SerializeAndLoad<sdsl::int_vector<64> >();
}

TEST_F(int_vector_test, iterator_test)
{
    for (auto i : vec_sizes) {
        sdsl::int_vector<> iv(i+3);
        sdsl::util::set_to_id(iv);
        sdsl::int_vector<>::iterator it = iv.begin();
        ASSERT_EQ(iv[0], *it++);
        ASSERT_EQ(iv[1], *it);
        it += 1;
        ASSERT_EQ(iv[2], *it);
        it -= 1;
        ASSERT_EQ(iv[1], *it);
        it -= -1;
        ASSERT_EQ(iv[2], *it);
        it += -1;
        ASSERT_EQ(iv[1], *it);
        ASSERT_EQ(iv[2], *(++it));
        it = iv.end()-1;
        ASSERT_EQ(iv[iv.size()-1], *it--);
        ASSERT_EQ(iv[iv.size()-2], *it);
        ASSERT_EQ(iv[iv.size()-3], *(--it));
    }
    for (auto i : vec_sizes) {
        sdsl::int_vector<> iv(i+3);
        sdsl::util::set_to_id(iv);
        sdsl::int_vector<>::const_iterator it(iv.begin());
        ASSERT_EQ(iv[0], *it++);
        ASSERT_EQ(iv[1], *it);
        it += 1;
        ASSERT_EQ(iv[2], *it);
        it -= 1;
        ASSERT_EQ(iv[1], *it);
        it -= -1;
        ASSERT_EQ(iv[2], *it);
        it += -1;
        ASSERT_EQ(iv[1], *it);
        ASSERT_EQ(iv[2], *(++it));
        it = iv.end()-1;
        ASSERT_EQ(iv[iv.size()-1], *it--);
        ASSERT_EQ(iv[iv.size()-2], *it);
        ASSERT_EQ(iv[iv.size()-3], *(--it));
    }
}

TEST_F(int_vector_test, growth_factor_test)
{
    std::vector<float> growth_factors{1.5, 2.0, 5.0, 10.0};
    for (float gw : growth_factors)
    {
        sdsl::int_vector<> v;
        v.resize(10);   // v is at max capacity
        v.growth_factor = gw;
        ASSERT_EQ(v.growth_factor, gw);
        uint64_t capacity_start = v.bit_capacity(); // number of reserved bits
        // size in bits of the vector when we want to push back one element
        uint64_t bit_size = v.width() * (v.size() + 1);
        // Get the resize factor
        auto resize_factor = pow(gw, std::ceil(std::log((bit_size + capacity_start - 1) / capacity_start) / std::log(gw)));
        // force resize
        v.push_back(0);
        uint64_t new_capacity = std::ceil(resize_factor * capacity_start);
        // the new capacity must be a multiple of 64
        new_capacity = ((new_capacity + 63) >> 6) << 6;
        ASSERT_EQ(v.bit_capacity(), new_capacity);
    }
}

}  // namespace

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    if (argc < 2) {
        // LCOV_EXCL_START
        std::cout << "Usage: " << argv[0] << " tmp_dir" << std::endl;
        return 1;
        // LCOV_EXCL_STOP
    }
    temp_dir = argv[1];
    return RUN_ALL_TESTS();
}
