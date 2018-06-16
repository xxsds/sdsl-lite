#include "common.hpp"
#include "sdsl/bit_vectors.hpp"
#include "sdsl/rank_support.hpp"
#include "gtest/gtest.h"
#include <string>

using namespace sdsl;
using namespace std;

string test_file;
string temp_file;
string temp_dir;

namespace
{

template<class T>
class rank_support_test : public ::testing::Test { };

using testing::Types;

typedef Types<@typedef_line@> Implementations;

TYPED_TEST_CASE(rank_support_test, Implementations);

//! Test the rank method
TYPED_TEST(rank_support_test, rank_method)
{
    static_assert(sdsl::util::is_regular<TypeParam>::value, "Type is not regular");
    bit_vector bvec;
    ASSERT_TRUE(load_from_file(bvec, test_file));
    typename TypeParam::bit_vector_type bv(bvec);
    TypeParam rs(&bv);
    uint64_t rank=0;
    for (uint64_t j=0; j < bvec.size(); ++j) {
        ASSERT_EQ(rank, rs.rank(j));
        bool found = (j >= TypeParam::bit_pat_len-1);
        for (uint8_t k=0; found and k < TypeParam::bit_pat_len; ++k) {
            found &= bvec[j-k] == ((TypeParam::bit_pat>>k)&1);
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

    uint64_t rank=0;
    for (uint64_t j=0; j < bvec.size(); ++j) {
        ASSERT_EQ(rank, c_rs.rank(j));
        bool found = (j >= TypeParam::bit_pat_len-1);
        for (uint8_t k=0; found and k < TypeParam::bit_pat_len; ++k) {
            found &= bvec[j-k] == ((TypeParam::bit_pat>>k)&1);
        }
        rank += found;
    }
    EXPECT_EQ(rank, c_rs.rank(bvec.size()));
    sdsl::remove(temp_file);
}

}// end namespace

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    if ( init_2_arg_test(argc, argv, "RANK_SUPPORT", test_file, temp_dir, temp_file) != 0 ) {
        return 1;
    }
    return RUN_ALL_TESTS();
}

