#include <algorithm> // for std::min
#include <random>
#include <string>
#include <vector>

#include <sdsl/wt_epr.hpp>

#include "common.hpp"

#include <gtest/gtest.h>

namespace
{

using namespace sdsl;
using namespace std;

typedef int_vector<>::size_type size_type;

string temp_file;
string temp_dir;

template <class T>
class wt_byte_epr_test : public ::testing::Test
{
protected:
    // Needs to be a member instead of a global since the static sdsl::memory_manager might call its destructor
    // before the vector.
    static int_vector<8> text;
};

template <class T>
int_vector<8> wt_byte_epr_test<T>::text{[]()
                                        {
                                            int_vector<8> result;
                                            result.resize(std::rand() % 10000);
                                            for (uint32_t i = 0; i < result.size(); ++i)
                                            {
                                                result[i] = (std::rand() % 3) + 1; // no 0s allowed. produces 1, 2 or 3.
                                            }
                                            return result;
                                        }()};

using testing::Types;

typedef Types<wt_epr<4>> Implementations;

TYPED_TEST_SUITE(wt_byte_epr_test, Implementations, );

TYPED_TEST(wt_byte_epr_test, create_and_store)
{
    static_assert(sdsl::util::is_regular<TypeParam>::value, "Type is not regular");

    TypeParam wt(this->text.begin(), this->text.end());

    ASSERT_TRUE(store_to_file(wt, temp_file));
}

//! Test sigma
TYPED_TEST(wt_byte_epr_test, sigma)
{
    TypeParam wt;
    ASSERT_TRUE(load_from_file(wt, temp_file));
    ASSERT_EQ(this->text.size(), wt.size());
    bit_vector occur(256, 0);
    uint16_t sigma = 0;
    for (size_type j = 0; j < this->text.size(); ++j)
    {
        if (!occur[(unsigned char)this->text[j]])
        {
            occur[(unsigned char)this->text[j]] = 1;
            ++sigma;
        }
    }
    ASSERT_EQ(sigma, wt.sigma);
}

template <class t_wt>
void compare_wt(int_vector<8> const & text, t_wt const & wt)
{
    ASSERT_EQ(text.size(), wt.size());
    for (size_type j = 0; j < text.size(); ++j)
    {
        ASSERT_EQ((typename t_wt::value_type)text[j], wt[j]) << " j=" << j;
    }
}

//! Test Access method, Copy-construtor, Move-constructor, Copy-assign and Move-assign
TYPED_TEST(wt_byte_epr_test, access_copy_move_and_swap)
{
    TypeParam wt;
    ASSERT_TRUE(load_from_file(wt, temp_file));
    compare_wt(this->text, wt);

    // Copy-constructor
    TypeParam wt2(wt);
    compare_wt(this->text, wt2);

    // Move-constructor
    TypeParam wt3(std::move(wt2));
    compare_wt(this->text, wt3);

    // Copy-Assign
    TypeParam wt4;
    wt4 = wt3;
    compare_wt(this->text, wt4);

    // Move-Assign
    TypeParam wt5;
    wt5 = std::move(wt4);
    compare_wt(this->text, wt5);
}

//! Test rank methods
TYPED_TEST(wt_byte_epr_test, rank)
{
    TypeParam wt;
    ASSERT_TRUE(load_from_file(wt, temp_file));

    ASSERT_EQ(this->text.size(), wt.size());

    // Test rank(i, c) for each character c and position i
    unsigned cnt_prefix_rank[4] = {0};
    for (unsigned i = 0; i < this->text.size() + 1; ++i)
    {
        for (unsigned v = 0; v < wt.sigma; ++v)
        {
            if (i > 0 && this->text[i - 1] <= v)
                ++cnt_prefix_rank[v];

            // auto const rank = rb(i, v);
            if (v > 0)
                ASSERT_EQ(cnt_prefix_rank[v] - cnt_prefix_rank[v - 1], wt.rank(i, v)) << " v=" << v;
            else
                ASSERT_EQ(cnt_prefix_rank[v], wt.rank(i, v)) << " v=" << v;

            if (v > 0)
            {
                auto lex = wt.lex_smaller_count(i, v);
                ASSERT_EQ(cnt_prefix_rank[v - 1], std::get<1>(lex)) << " v=" << v;
            }
        }
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

    {
        TypeParam in_l{};
        std::ifstream is{temp_file, std::ios::binary};
        in_archive_t iarchive{is};
        iarchive(in_l);
        EXPECT_EQ(l, in_l);
    }
}

TYPED_TEST(wt_byte_epr_test, cereal)
{
    if (temp_dir != "@/")
    {
        TypeParam wt;
        ASSERT_TRUE(load_from_file(wt, temp_file));

        do_serialisation<cereal::BinaryInputArchive, cereal::BinaryOutputArchive>(wt);
        do_serialisation<cereal::PortableBinaryInputArchive, cereal::PortableBinaryOutputArchive>(wt);
        do_serialisation<cereal::JSONInputArchive, cereal::JSONOutputArchive>(wt);
        do_serialisation<cereal::XMLInputArchive, cereal::XMLOutputArchive>(wt);
    }
}
#endif // SDSL_HAS_CEREAL

TYPED_TEST(wt_byte_epr_test, delete_)
{
    sdsl::remove(temp_file);
}

} // namespace

int main(int argc, char ** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    if (argc < 2)
    {
        // LCOV_EXCL_START
        std::cout << "Usage: " << argv[0] << " tmp_dir" << std::endl;
        return 1;
        // LCOV_EXCL_STOP
    }
    temp_dir = argv[1];
    temp_file = temp_dir + "/wt_epr";

    auto const seed{time(NULL)};
    srand(seed);

    return RUN_ALL_TESTS();
}
