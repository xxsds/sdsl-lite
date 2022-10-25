#include <string>
#include <type_traits>
#include <vector>

#include <sdsl/coder_elias_gamma.hpp>
#include <sdsl/coder_fibonacci.hpp>
#include <sdsl/csa_bitcompressed.hpp>
#include <sdsl/csa_sada.hpp>
#include <sdsl/csa_wt.hpp>
#include <sdsl/suffix_array_algorithm.hpp>

#include "common.hpp"

#include <gtest/gtest.h>

namespace
{

using namespace sdsl;
using namespace std;

typedef int_vector<>::size_type size_type;

tMSS test_case_file_map;
string test_file;
string temp_file;
string temp_dir;

template <class T>
class csa_byte_test : public ::testing::Test
{};

using testing::Types;

#ifdef FULL_TEST_SUITE

typedef Types<csa_wt<>,
              csa_sada<>,
              csa_bitcompressed<>,
              csa_sada<enc_vector<coder::fibonacci<>>>,
              csa_sada<enc_vector<coder::elias_gamma<>>>,
              csa_wt<wt_huff<>, 8, 16, text_order_sa_sampling<>>,
              csa_wt<wt_huff<>, 32, 32, fuzzy_sa_sampling<>>,
              csa_wt<wt_huff<>, 32, 32, fuzzy_sa_sampling<bit_vector, bit_vector>, fuzzy_isa_sampling_support<>>,
              csa_wt<wt_huff<>, 32, 32, fuzzy_sa_sampling<>, fuzzy_isa_sampling_support<>>,
              csa_wt<wt_huff<>, 32, 32, text_order_sa_sampling<>, isa_sampling<>>,
              csa_wt<wt_huff<>, 32, 32, text_order_sa_sampling<>, text_order_isa_sampling_support<>>,
              csa_sada<enc_vector<>, 32, 32, text_order_sa_sampling<>, isa_sampling<>>,
              csa_sada<enc_vector<>, 32, 32, text_order_sa_sampling<>, text_order_isa_sampling_support<>>,
              csa_wt<wt_huff<>, 8, 16, sa_order_sa_sampling<>>,
              csa_wt<wt_huff<>,
                     8,
                     16,
                     sa_order_sa_sampling<>,
                     isa_sampling<>,
                     succinct_byte_alphabet<bit_vector, rank_support_v<>, select_support_mcl<>>>,
              csa_wt<wt_huff<>, 8, 16, sa_order_sa_sampling<>, isa_sampling<>, succinct_byte_alphabet<>>,
              csa_wt<wt_huff<>, 8, 16, sa_order_sa_sampling<>, isa_sampling<>, plain_byte_alphabet>>
    Implementations;

#else

typedef Types<csa_wt<>, csa_sada<>, csa_bitcompressed<>> Implementations;

#endif

TYPED_TEST_SUITE(csa_byte_test, Implementations, );

TYPED_TEST(csa_byte_test, create_and_store_test)
{
    static_assert(sdsl::util::is_regular<TypeParam>::value, "Type is not regular");
    TypeParam csa;
    std::string temp_file2 = sdsl::tmp_file(temp_dir + "/" + util::basename(test_file), util::basename(test_file));
    cache_config config(false, temp_dir, util::basename(temp_file2));
    construct(csa, test_file, config, 1);
    test_case_file_map = config.file_map;
    ASSERT_TRUE(store_to_file(csa, temp_file));
}

//! Test backward_search
TYPED_TEST(csa_byte_test, backward_search)
{
    TypeParam csa;
    ASSERT_TRUE(load_from_file(csa, temp_file));
    int_vector<8> text;
    ASSERT_TRUE(load_vector_from_file(text, test_file, 1));
    auto expected_interval_member = csa.psi[0];
    size_type count, l_res, r_res;
    // search for full text
    count = backward_search(csa, 0, csa.size() - 1, text.begin(), text.end(), l_res, r_res);
    ASSERT_EQ((size_type)1, count);
    ASSERT_EQ(l_res, expected_interval_member);
    ASSERT_EQ(r_res, expected_interval_member);
    // search for short phrase
    text.resize(min((int_vector<8>::size_type)4, text.size()));
    count = backward_search(csa, 0, csa.size() - 1, text.begin(), text.end(), l_res, r_res);
    ASSERT_LE((size_type)1, count);
    ASSERT_LE(l_res, expected_interval_member);
    ASSERT_GE(r_res, expected_interval_member);
    // search for empty phrase
    text.resize(0);
    count = backward_search(csa, 0, csa.size() - 1, text.begin(), text.end(), l_res, r_res);
    ASSERT_EQ(csa.size(), count);
    ASSERT_EQ(l_res, (size_type)0);
    ASSERT_EQ(r_res, (size_type)(csa.size() - 1));
}

//! Test forward_search
TYPED_TEST(csa_byte_test, forward_search)
{
    TypeParam csa;
    ASSERT_TRUE(load_from_file(csa, temp_file));
    int_vector<8> text;
    ASSERT_TRUE(load_vector_from_file(text, test_file, 1));
    auto expected_interval_member = csa.psi[0];
    size_type count, l_res, r_res;
    // search for full text
    count = forward_search(csa, 0, csa.size() - 1, text.begin(), text.end(), l_res, r_res);
    ASSERT_EQ((size_type)1, count);
    ASSERT_EQ(l_res, expected_interval_member);
    ASSERT_EQ(r_res, expected_interval_member);
    // search for short phrase
    text.resize(min((int_vector<8>::size_type)4, text.size()));
    count = forward_search(csa, 0, csa.size() - 1, text.begin(), text.end(), l_res, r_res);
    ASSERT_LE((size_type)1, count);
    ASSERT_LE(l_res, expected_interval_member);
    ASSERT_GE(r_res, expected_interval_member);
    // search for empty phrase
    text.resize(0);
    count = forward_search(csa, 0, csa.size() - 1, text.begin(), text.end(), l_res, r_res);
    ASSERT_EQ(csa.size(), count);
    ASSERT_EQ(l_res, (size_type)0);
    ASSERT_EQ(r_res, (size_type)(csa.size() - 1));
}

//! Test sigma member
TYPED_TEST(csa_byte_test, sigma)
{
    TypeParam csa;
    ASSERT_TRUE(load_from_file(csa, temp_file));
    int_vector<8> text;
    ASSERT_TRUE(load_vector_from_file(text, test_file, 1));
    text.resize(text.size() + 1);
    text[text.size() - 1] = 0; // add 0-character to the end
    ASSERT_EQ(text.size(), csa.size());

    if (!std::is_same<typename TypeParam::alphabet_type, plain_byte_alphabet>::value)
    {
        bit_vector occur(256, 0);
        uint16_t sigma = 0;
        for (size_type j = 0; j < text.size(); ++j)
        {
            if (!occur[text[j]])
            {
                occur[text[j]] = 1;
                ++sigma;
            }
        }
        ASSERT_EQ(sigma, csa.sigma);
    }
    else
    {
        int max_char = 0;
        for (int chr : text)
            max_char = std::max(max_char, chr);

        ASSERT_EQ(max_char + 1, csa.sigma);
    }
}

//! Test suffix array access methods
TYPED_TEST(csa_byte_test, sa_access)
{
    TypeParam csa;
    ASSERT_TRUE(load_from_file(csa, temp_file));
    int_vector<> sa;
    load_from_file(sa, test_case_file_map[conf::KEY_SA]);
    size_type n = sa.size();
    ASSERT_EQ(n, csa.size());
    for (size_type j = 0; j < n; ++j)
    {
        ASSERT_EQ(sa[j], csa[j]) << " j=" << j;
    }
}

//! Test inverse suffix access methods
TYPED_TEST(csa_byte_test, isa_access)
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
        for (size_type j = 0; j < n; ++j)
        {
            isa[sa[j]] = j;
        }
    }
    for (size_type j = 0; j < n; ++j)
    {
        ASSERT_EQ(isa[j], csa.isa[j]) << " j=" << j;
    }
}

//! Test text access methods
TYPED_TEST(csa_byte_test, text_access)
{
    if (test_case_file_map.find(conf::KEY_TEXT) != test_case_file_map.end())
    {
        TypeParam csa;
        ASSERT_TRUE(load_from_file(csa, temp_file));
        int_vector<8> text;
        load_from_file(text, test_case_file_map[conf::KEY_TEXT]);
        size_type n = text.size();
        ASSERT_EQ(n, csa.size());
        for (size_type j = 0; j < n; ++j)
        {
            ASSERT_EQ(text[j], csa.text[j]) << " j=" << j;
        }
        auto len = std::min(csa.size(), std::max(csa.size() / 10, (decltype(csa.size()))20));
        auto ex_text = extract(csa, 0, len - 1);
        for (size_type j = 0; j < len; ++j)
        {
            ASSERT_EQ(text[j], (decltype(text[j]))ex_text[j]) << " j=" << j;
        }
    }
}

//! Test Burrows-Wheeler access methods
TYPED_TEST(csa_byte_test, bwt_access)
{
    if (test_case_file_map.find(conf::KEY_BWT) != test_case_file_map.end())
    {
        TypeParam csa;
        ASSERT_TRUE(load_from_file(csa, temp_file));
        int_vector<8> bwt;
        load_from_file(bwt, test_case_file_map[conf::KEY_BWT]);
        size_type n = bwt.size();
        ASSERT_EQ(n, csa.size());
        for (size_type j = 0; j < n; ++j)
        {
            ASSERT_EQ(bwt[j], csa.bwt[j]) << " j=" << j;
        }
    }
}

TYPED_TEST(csa_byte_test, f_access)
{
    if (test_case_file_map.find(conf::KEY_TEXT) != test_case_file_map.end())
    {
        TypeParam csa;
        ASSERT_TRUE(load_from_file(csa, temp_file));
        int_vector<8> text;
        load_from_file(text, test_case_file_map[conf::KEY_TEXT]);
        std::sort(begin(text), end(text));
        size_type n = text.size();
        ASSERT_EQ(n, csa.size());
        for (size_type j = 0; j < n; j += 200)
        {
            ASSERT_EQ(text[j], csa.F[j]) << " j=" << j;
        }
    }
}

//! Test Psi access methods
TYPED_TEST(csa_byte_test, psi_access)
{
    if (test_case_file_map.find(conf::KEY_PSI) != test_case_file_map.end())
    {
        TypeParam csa;
        ASSERT_TRUE(load_from_file(csa, temp_file));
        int_vector<> psi;
        load_from_file(psi, test_case_file_map[conf::KEY_PSI]);
        size_type n = psi.size();
        ASSERT_EQ(n, csa.size());
        for (size_type j = 0; j < n; ++j)
        {
            ASSERT_EQ(psi[j], csa.psi[j]) << " j=" << j;
        }
    }
}

//! Test if Psi[LF[i]]=i
TYPED_TEST(csa_byte_test, psi_lf_access)
{
    TypeParam csa;
    ASSERT_TRUE(load_from_file(csa, temp_file));
    for (size_type j = 0; j < csa.size(); ++j)
    {
        size_type lf = csa.lf[j];
        ASSERT_TRUE(lf < csa.size());
        ASSERT_EQ(j, csa.psi[lf]) << " j=" << j;
    }
}

//! Test access after swap
TYPED_TEST(csa_byte_test, swap)
{
    TypeParam csa1;
    ASSERT_TRUE(load_from_file(csa1, temp_file));
    TypeParam csa2;
    {
        TypeParam tmp = csa1;
        csa1 = std::move(csa2);
        csa2 = std::move(tmp);
    }
    int_vector<> sa;
    load_from_file(sa, test_case_file_map[conf::KEY_SA]);
    size_type n = sa.size();
    ASSERT_EQ(n, csa2.size());
    for (size_type j = 0; j < n; ++j)
    {
        ASSERT_EQ((typename TypeParam::value_type)sa[j], csa2[j]);
    }
}

#if SDSL_HAS_CEREAL
template <typename in_archive_t, typename out_archive_t, typename TypeParam>
void do_serialisation(TypeParam const & l)
{
    {
        std::ofstream os{temp_file, std::ios::binary};
        out_archive_t oarchive{os};
        oarchive(l);
    }

    TypeParam in_l{};
    {
        std::ifstream is{temp_file, std::ios::binary};
        in_archive_t iarchive{is};
        iarchive(in_l);
    }
    EXPECT_EQ(l, in_l);
}

TYPED_TEST(csa_byte_test, cereal)
{
    if (temp_dir != "@/")
    {
        TypeParam csa;
        ASSERT_TRUE(load_from_file(csa, temp_file));

        do_serialisation<cereal::BinaryInputArchive, cereal::BinaryOutputArchive>(csa);
        do_serialisation<cereal::PortableBinaryInputArchive, cereal::PortableBinaryOutputArchive>(csa);
        do_serialisation<cereal::JSONInputArchive, cereal::JSONOutputArchive>(csa);
        do_serialisation<cereal::XMLInputArchive, cereal::XMLOutputArchive>(csa);
    }
}
#endif // SDSL_HAS_CEREAL

TYPED_TEST(csa_byte_test, delete_)
{
    sdsl::remove(temp_file);
    util::delete_all_files(test_case_file_map);
}

} // namespace

int main(int argc, char ** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    if (init_2_arg_test(argc, argv, "CSA_BYTE", test_file, temp_dir, temp_file) != 0)
    {
        return 1;
    }
    return RUN_ALL_TESTS();
}
