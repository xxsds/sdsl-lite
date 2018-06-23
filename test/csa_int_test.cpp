#include "common.hpp"
#include "sdsl/suffix_arrays.hpp"
#include "gtest/gtest.h"
#include <cstdlib>
#include <vector>
#include <string>
#include <algorithm>

namespace
{

using namespace sdsl;
using namespace std;

typedef int_vector<>::size_type size_type;
tMSS    test_case_file_map;
string  test_file;
uint8_t num_bytes;
string  temp_file;
string  temp_dir;


template<class T>
class csa_int_test : public ::testing::Test { };


using testing::Types;

#ifdef FULL_TEST_SUITE

typedef Types<
    csa_wt<wt_int<>, 32, 32, sa_order_sa_sampling<>, isa_sampling<>, int_alphabet<>>,
    csa_sada<enc_vector<>, 32, 32, sa_order_sa_sampling<>, isa_sampling<>, int_alphabet<>>,
    csa_bitcompressed<int_alphabet<>>,
    csa_wt<wt_int<rrr_vector<63> >, 8, 8, sa_order_sa_sampling<>, isa_sampling<>, int_alphabet<>>,
    csa_wt<wt_int<>, 16, 16, text_order_sa_sampling<>, text_order_isa_sampling_support<>, int_alphabet<>>,
    csa_sada<enc_vector<>, 32, 32, text_order_sa_sampling<>, isa_sampling<>, int_alphabet<>>
> Implementations;

#else

typedef Types<
    csa_wt<wt_int<>, 32, 32, sa_order_sa_sampling<>, isa_sampling<>, int_alphabet<>>,
    csa_sada<enc_vector<>, 32, 32, sa_order_sa_sampling<>, isa_sampling<>, int_alphabet<>>,
    csa_bitcompressed<int_alphabet<>>
> Implementations;

#endif

TYPED_TEST_CASE(csa_int_test, Implementations);

TYPED_TEST(csa_int_test, create_and_store)
{
    static_assert(sdsl::util::is_regular<TypeParam>::value, "Type is not regular");
    TypeParam csa;
    std::string temp_file2 = sdsl::tmp_file(temp_dir+"/"+util::basename(test_file),util::basename(test_file));
    cache_config config(false, temp_dir, util::basename(temp_file2));
    construct(csa, test_file, config, num_bytes);
    test_case_file_map = config.file_map;
    ASSERT_TRUE(store_to_file(csa, temp_file));
}

//! Test access methods
TYPED_TEST(csa_int_test, sigma)
{
    TypeParam csa;
    ASSERT_TRUE(load_from_file(csa, temp_file));
    int_vector<> text;
    load_vector_from_file(text, test_file, num_bytes);
    text.resize(text.size()+1);
    text[text.size()-1] = 0;    // add 0-character at the end
    size_type n = text.size();
    ASSERT_EQ(n, csa.size());
    std::set<uint64_t> occur;
    size_type sigma = 0;
    for (size_type j=0; j<n; ++j) {
        if (occur.end() == occur.find(text[j])) {
            occur.insert(text[j]);
            ++sigma;
        }
    }
    ASSERT_EQ(sigma, csa.sigma);
}

//! Test suffix array access methods
TYPED_TEST(csa_int_test, sa_access)
{
    TypeParam csa;
    ASSERT_TRUE(load_from_file(csa, temp_file));
    int_vector<> sa;
    load_from_file(sa, test_case_file_map[conf::KEY_SA]);
    size_type n = sa.size();
    ASSERT_EQ(n, csa.size());
    for (size_type j=0; j<n; ++j) {
        ASSERT_EQ(sa[j], csa[j])<<" j="<<j;
    }
}


//! Test inverse suffix access methods
TYPED_TEST(csa_int_test, isa_access)
{
    TypeParam csa;
    ASSERT_TRUE(load_from_file(csa, temp_file));
    int_vector<> isa;
    size_type n = 0;
    {
        int_vector<> sa;
        load_from_file(sa, test_case_file_map[conf::KEY_SA]);
        n = sa.size();
        ASSERT_EQ(n, csa.size());
        isa = sa;
        for (size_type j=0; j<n; ++j) {
            isa[sa[j]] = j;    // calculate inverse suffix array
        }
    }
    for (size_type j=0; j<n; ++j) {
        ASSERT_EQ(isa[j], csa.isa[j])<<" j="<<j;
    }
}

//! Test Burrows-Wheeler access methods
TYPED_TEST(csa_int_test, bwt_access)
{
    if (test_case_file_map.end() != test_case_file_map.find(conf::KEY_BWT_INT)) {
        TypeParam csa;
        ASSERT_TRUE(load_from_file(csa, temp_file));
        int_vector<> bwt;
        load_from_file(bwt, test_case_file_map[conf::KEY_BWT_INT]);
        size_type n = bwt.size();
        ASSERT_EQ(n, csa.size());
        for (size_type j=0; j<n; ++j) {
            ASSERT_EQ(bwt[j], csa.bwt[j])<<" j="<<j;
        }
    }
}

TYPED_TEST(csa_int_test, f_access)
{
    if (test_case_file_map.end() != test_case_file_map.find(conf::KEY_TEXT_INT)) {
        TypeParam csa;
        ASSERT_TRUE(load_from_file(csa, temp_file));
        int_vector<> text;
        load_from_file(text, test_case_file_map[conf::KEY_TEXT_INT]);
        std::sort(begin(text),end(text));
        size_type n = text.size();
        ASSERT_EQ(n, csa.size());
        for (size_type j=0; j<n; j+=200) {
            ASSERT_EQ(text[j], csa.F[j])<<" j="<<j;
        }
    }
}

//! Test text access methods
TYPED_TEST(csa_int_test, text_access)
{
    if (test_case_file_map.end() != test_case_file_map.find(conf::KEY_TEXT_INT)) {
        TypeParam csa;
        ASSERT_TRUE(load_from_file(csa, temp_file));
        int_vector<> text;
        load_from_file(text, test_case_file_map[conf::KEY_TEXT_INT]);
        size_type n = text.size();
        ASSERT_EQ(n, csa.size());
        for (size_type j=0; j<n; ++j) {
            ASSERT_EQ(text[j], csa.text[j])<<" j="<<j;
        }
        auto len = std::min(csa.size(),
                            std::max(csa.size()/10, (decltype(csa.size()))20));
        auto ex_text = extract(csa, 0, len-1);
        for (size_type j=0; j<len; ++j) {
            ASSERT_EQ(text[j], ex_text[j])<<" j="<<j;
        }
        if (n > 0) {
            auto c_out_of_range = (*std::max_element(text.begin(), text.end()))+1;
            auto cnt = count(csa, {c_out_of_range});
            ASSERT_EQ(0ULL, cnt) << " c_out_of_range="<<c_out_of_range<<" text="<<csa.text;
        }
    }
}

//! Test Psi access methods
TYPED_TEST(csa_int_test, psi_access)
{
    if (test_case_file_map.end() != test_case_file_map.find(conf::KEY_PSI)) {
        TypeParam csa;
        ASSERT_TRUE(load_from_file(csa, temp_file));
        int_vector<> psi;
        load_from_file(psi, test_case_file_map[conf::KEY_PSI]);
        size_type n = psi.size();
        ASSERT_EQ(n, csa.size());
        for (size_type j=0; j<n; ++j) {
            ASSERT_EQ(psi[j], csa.psi[j])<<" j="<<j;
        }
    }
}

//! Test if Psi[LF[i]]=i
TYPED_TEST(csa_int_test, psi_lf_access)
{
    TypeParam csa;
    ASSERT_TRUE(load_from_file(csa, temp_file));
    for (size_type j=0; j<csa.size(); ++j) {
        size_type lf = csa.lf[j];
        ASSERT_TRUE(lf < csa.size());
        ASSERT_EQ(j, csa.psi[lf])<<" j="<<j;
    }
}


//! Test access after swap
TYPED_TEST(csa_int_test, swap_test)
{
    TypeParam csa1;
    ASSERT_TRUE(load_from_file(csa1, temp_file));
    TypeParam csa2;
    std::swap(csa1, csa2);
    int_vector<> sa;
    load_from_file(sa, test_case_file_map[conf::KEY_SA]);
    size_type n = sa.size();
    ASSERT_EQ(n, csa2.size());
    for (size_type j=0; j<n; ++j) {
        ASSERT_EQ((typename TypeParam::value_type)sa[j], csa2[j]);
    }
}

TYPED_TEST(csa_int_test, delete_)
{
    sdsl::remove(temp_file);
    util::delete_all_files(test_case_file_map);
}

}  // namespace

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    if ( init_3_arg_test(argc, argv, "CSA_INT", test_file, num_bytes, temp_dir, temp_file) == 1 ) {
        return 1;
    }
    return RUN_ALL_TESTS();
}
