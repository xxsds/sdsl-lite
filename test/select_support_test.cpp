#include "common.hpp"
#include "sdsl/bit_vectors.hpp"
#include "sdsl/select_support.hpp"
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
class select_support_test : public ::testing::Test { };

using testing::Types;

typedef Types<@typedef_line@> Implementations;

TYPED_TEST_CASE(select_support_test, Implementations);

//! Test the select method
TYPED_TEST(select_support_test, select_method)
{
    static_assert(sdsl::util::is_regular<TypeParam>::value, "Type is not regular");
    bit_vector bvec;
    ASSERT_TRUE(load_from_file(bvec, test_file));
    typename TypeParam::bit_vector_type bv(bvec);
    TypeParam ss(&bv);
    for (uint64_t j=0, select=0; j < bvec.size(); ++j) {
        bool found = (j >= TypeParam::bit_pat_len-1);
        for (uint8_t k=0; found and k < TypeParam::bit_pat_len; ++k) {
            found &= bvec[j-k] == ((TypeParam::bit_pat>>k)&1);
        }
        if (found) {
            ++select;
            ASSERT_EQ(j, ss.select(select));
        }
    }
}

}// end namespace

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    if ( init_2_arg_test(argc, argv, "SELECT_SUPPORT", test_file, temp_dir, temp_file) != 0 ) {
        return 1;
    }
    return RUN_ALL_TESTS();
}
