#include <string>

#include "sdsl/bit_vector_il.hpp"
#include "sdsl/hyb_vector.hpp"
#include "sdsl/rank_support_scan.hpp"
#include "sdsl/rank_support_v.hpp"
#include "sdsl/rank_support_v5.hpp"
#include "sdsl/rrr_vector.hpp"
#include "sdsl/sd_vector.hpp"

#include "common.hpp"

#include <gtest/gtest.h>

using namespace sdsl;
using namespace std;

string test_file;
string temp_file;
string temp_dir;

namespace
{

template <class T>
class rank_support_test : public ::testing::Test
{};

using testing::Types;

#ifdef FULL_TEST_SUITE

typedef Types<rank_support_il<1, 512>,
              rank_support_v<>,
              rank_support_v5<>,
              rank_support_sd<1>,
              rank_support_il<0, 512>,
              rank_support_rrr<0>,
              rank_support_v<0>,
              rank_support_v5<0>,
              rank_support_sd<0>,
              rank_support_hyb<1>,
              rank_support_hyb<0>,
              rank_support_v<10, 2>,
              rank_support_v<01, 2>,
              rank_support_v<00, 2>,
              rank_support_v<11, 2>,
              rank_support_v5<10, 2>,
              rank_support_v5<01, 2>,
              rank_support_v5<00, 2>,
              rank_support_v5<11, 2>,
              rank_support_il<1, 256>,
              rank_support_il<1, 1024>,
              rank_support_rrr<1, 64>,
              rank_support_rrr<1, 192>,
              rank_support_rrr<1, 256>,
              rank_support_rrr<1, 255>,
              rank_support_rrr<1, 15>,
              rank_support_rrr<1, 31>,
              rank_support_rrr<1, 63>,
              rank_support_rrr<1, 83>,
              rank_support_rrr<1, 127>,
              rank_support_rrr<1, 128>,
              rank_support_rrr<1, 129>,
              rank_support_il<0, 256>,
              rank_support_il<0, 1024>,
              rank_support_rrr<0, 64>,
              rank_support_rrr<0, 192>,
              rank_support_rrr<0, 256>,
              rank_support_rrr<0, 255>,
              rank_support_rrr<0, 15>,
              rank_support_rrr<0, 30>,
              rank_support_rrr<0, 63>,
              rank_support_rrr<0, 83>,
              rank_support_rrr<0, 127>,
              rank_support_rrr<0, 128>,
              rank_support_rrr<0, 129>>
                                                  Implementations;

#else

typedef Types<rank_support_il<1, 512>,
              rank_support_rrr<>,
              rank_support_v<>,
              rank_support_v5<>,
              rank_support_sd<1>,
              rank_support_il<0, 512>,
              rank_support_rrr<0>,
              rank_support_v<0>,
              rank_support_v5<0>,
              rank_support_sd<0>,
              rank_support_hyb<1>,
              rank_support_hyb<0>,
              rank_support_v<10, 2>,
              rank_support_v<01, 2>,
              rank_support_v<00, 2>,
              rank_support_v<11, 2>,
              rank_support_v5<10, 2>,
              rank_support_v5<01, 2>,
              rank_support_v5<00, 2>,
              rank_support_v5<11, 2>>
                                                  Implementations;

#endif

TYPED_TEST_SUITE(rank_support_test, Implementations, );

//! Test the rank method
TYPED_TEST(rank_support_test, rank_method)
{
    static_assert(sdsl::util::is_regular<TypeParam>::value, "Type is not regular");
    bit_vector bvec;
    ASSERT_TRUE(load_from_file(bvec, test_file));
    typename TypeParam::bit_vector_type bv(bvec);
    TypeParam rs(&bv);
    uint64_t rank = 0;
    for (uint64_t j = 0; j < bvec.size(); ++j)
    {
        ASSERT_EQ(rank, rs.rank(j));
        bool found = (j >= TypeParam::bit_pat_len - 1);
        for (uint8_t k = 0; found and k < TypeParam::bit_pat_len; ++k)
        {
            found &= bvec[j - k] == ((TypeParam::bit_pat >> k) & 1);
        }
        rank += found;
    }
    EXPECT_EQ(rank, rs.rank(bvec.size()));
}

//! Test the rank method
TYPED_TEST(rank_support_test, store_and_load)
{
    bit_vector bvec;
    ASSERT_TRUE(load_from_file(bvec, test_file));
    typename TypeParam::bit_vector_type bv(bvec);
    TypeParam rs(&bv);
    TypeParam c_rs(&bv);
    ASSERT_TRUE(store_to_file(rs, temp_file));
    ASSERT_TRUE(load_from_file(c_rs, temp_file));
    c_rs.set_vector(&bv);

    uint64_t rank = 0;
    for (uint64_t j = 0; j < bvec.size(); ++j)
    {
        ASSERT_EQ(rank, c_rs.rank(j));
        bool found = (j >= TypeParam::bit_pat_len - 1);
        for (uint8_t k = 0; found and k < TypeParam::bit_pat_len; ++k)
        {
            found &= bvec[j - k] == ((TypeParam::bit_pat >> k) & 1);
        }
        rank += found;
    }
    EXPECT_EQ(rank, c_rs.rank(bvec.size()));
    sdsl::remove(temp_file);
}

#if SDSL_HAS_CEREAL
template <typename in_archive_t, typename out_archive_t, typename TypeParam>
void do_serialisation(TypeParam const & l, typename TypeParam::bit_vector_type const & bv)
{
    {
        std::ofstream os{ temp_file, std::ios::binary };
        out_archive_t oarchive{ os };
        oarchive(l);
    }

    {
        TypeParam in_l{};
        std::ifstream is{ temp_file, std::ios::binary };
        in_archive_t iarchive{ is };
        iarchive(in_l);
        in_l.set_vector(&bv);
        EXPECT_EQ(l, in_l);
    }
}

TYPED_TEST(rank_support_test, cereal)
{
    if (temp_dir != "@/")
    {
        bit_vector bvec;
        ASSERT_TRUE(load_from_file(bvec, test_file));
        typename TypeParam::bit_vector_type bv(bvec);
        TypeParam rs(&bv);

        do_serialisation<cereal::BinaryInputArchive, cereal::BinaryOutputArchive>(rs, bv);
        do_serialisation<cereal::PortableBinaryInputArchive, cereal::PortableBinaryOutputArchive>(rs, bv);
        do_serialisation<cereal::JSONInputArchive, cereal::JSONOutputArchive>(rs, bv);
        do_serialisation<cereal::XMLInputArchive, cereal::XMLOutputArchive>(rs, bv);
    }
}
#endif // SDSL_HAS_CEREAL

} // end namespace

int main(int argc, char ** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    if (init_2_arg_test(argc, argv, "RANK_SUPPORT", test_file, temp_dir, temp_file) != 0) { return 1; }
    return RUN_ALL_TESTS();
}
