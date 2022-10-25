#include <stack>
#include <string>
#include <vector>

#include <sdsl/rmq_succinct_sada.hpp>
#include <sdsl/rmq_succinct_sct.hpp>

#include "common.hpp"

#include <gtest/gtest.h>

using namespace std;
using namespace sdsl;

namespace
{

typedef vector<uint64_t> tVUI;

string test_file;
string temp_dir;
string temp_file;

template <class T>
class rmq_test : public ::testing::Test
{};

using testing::Types;

typedef Types<sdsl::rmq_succinct_sct<>, sdsl::rmq_succinct_sada<>> Implementations;

TYPED_TEST_SUITE(rmq_test, Implementations, );

TYPED_TEST(rmq_test, construct_and_store)
{
    static_assert(sdsl::util::is_regular<TypeParam>::value, "Type is not regular");
    int_vector<> v;
    load_from_file(v, test_file);
    TypeParam rmq(&v);
    ASSERT_TRUE(sdsl::store_to_file(rmq, temp_file));
}

// helper class for next test
class state
{
public:
    uint64_t l, r; // left and right border of interval
    uint64_t idx;  // index of the min value
    uint64_t min;  // min value in the interval
    state(uint64_t fl = 0, uint64_t fr = 0, uint64_t fidx = 0, uint64_t fmin = 0) : l(fl), r(fr), idx(fidx), min(fmin)
    {}
};

//! Test range minimum queries
TYPED_TEST(rmq_test, rmq_load_and_query)
{
    int_vector<> v;
    ASSERT_TRUE(load_from_file(v, test_file));
    TypeParam rmq;
    ASSERT_TRUE(load_from_file(rmq, temp_file));
    ASSERT_EQ(v.size(), rmq.size());
    if (rmq.size() > 0)
    {
        stack<state> s;
        uint64_t idx = rmq(0, rmq.size() - 1);
        ASSERT_TRUE(idx < rmq.size());
        s.push(state(0, rmq.size() - 1, idx, v[idx]));
        while (!s.empty())
        {
            state st = s.top();
            s.pop();
            if (st.l < st.idx)
            {
                idx = rmq(st.l, st.idx - 1);
                ASSERT_TRUE(idx >= st.l);
                ASSERT_TRUE(idx <= st.idx - 1);
                ASSERT_TRUE(v[idx] >= v[st.idx]) << "v[" << idx << "]=" << v[idx] << " < "
                                                 << "v[" << st.idx << "]=" << v[st.idx] << endl
                                                 << "[" << st.l << "," << st.r << "]" << endl;
                s.push(state(st.l, st.idx - 1, idx, v[idx]));
            }
            if (st.idx < st.r)
            {
                idx = rmq(st.idx + 1, st.r);
                ASSERT_TRUE(idx >= st.idx + 1);
                ASSERT_TRUE(idx <= st.r);
                ASSERT_TRUE(v[idx] >= v[st.idx]) << "v[" << idx << "]=" << v[idx] << " < "
                                                 << "v[" << st.idx << "]=" << v[st.idx] << endl
                                                 << "[" << st.l << "," << st.r << "]" << endl;
                s.push(state(st.idx + 1, st.r, idx, v[idx]));
            }
        }
    }
}

//! Test range minimum queries
TYPED_TEST(rmq_test, rmq_load_and_move_and_query)
{
    int_vector<> v;
    ASSERT_TRUE(load_from_file(v, test_file));
    TypeParam rmq_load;
    ASSERT_TRUE(load_from_file(rmq_load, temp_file));
    TypeParam rmq = std::move(rmq_load);
    ASSERT_EQ(v.size(), rmq.size());
    if (rmq.size() > 0)
    {
        stack<state> s;
        uint64_t idx = rmq(0, rmq.size() - 1);
        ASSERT_TRUE(idx < rmq.size());
        s.push(state(0, rmq.size() - 1, idx, v[idx]));
        while (!s.empty())
        {
            state st = s.top();
            s.pop();
            if (st.l < st.idx)
            {
                idx = rmq(st.l, st.idx - 1);
                ASSERT_TRUE(idx >= st.l);
                ASSERT_TRUE(idx <= st.idx - 1);
                ASSERT_TRUE(v[idx] >= v[st.idx]) << "v[" << idx << "]=" << v[idx] << " < "
                                                 << "v[" << st.idx << "]=" << v[st.idx] << endl
                                                 << "[" << st.l << "," << st.r << "]" << endl;
                s.push(state(st.l, st.idx - 1, idx, v[idx]));
            }
            if (st.idx < st.r)
            {
                idx = rmq(st.idx + 1, st.r);
                ASSERT_TRUE(idx >= st.idx + 1);
                ASSERT_TRUE(idx <= st.r);
                ASSERT_TRUE(v[idx] >= v[st.idx]) << "v[" << idx << "]=" << v[idx] << " < "
                                                 << "v[" << st.idx << "]=" << v[st.idx] << endl
                                                 << "[" << st.l << "," << st.r << "]" << endl;
                s.push(state(st.idx + 1, st.r, idx, v[idx]));
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

    TypeParam in_l{};
    {
        std::ifstream is{temp_file, std::ios::binary};
        in_archive_t iarchive{is};
        iarchive(in_l);
    }
    EXPECT_EQ(l, in_l);
}

TYPED_TEST(rmq_test, cereal)
{
    if (temp_dir != "@/")
    {
        TypeParam rmq;
        ASSERT_TRUE(load_from_file(rmq, temp_file));

        do_serialisation<cereal::BinaryInputArchive, cereal::BinaryOutputArchive>(rmq);
        do_serialisation<cereal::PortableBinaryInputArchive, cereal::PortableBinaryOutputArchive>(rmq);
        do_serialisation<cereal::JSONInputArchive, cereal::JSONOutputArchive>(rmq);
        do_serialisation<cereal::XMLInputArchive, cereal::XMLOutputArchive>(rmq);
    }
}
#endif // SDSL_HAS_CEREAL

TYPED_TEST(rmq_test, delete_)
{
    sdsl::remove(temp_file);
}

} // namespace

int main(int argc, char ** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    init_2_arg_test(argc, argv, "RMQ", test_file, temp_dir, temp_file);
    return RUN_ALL_TESTS();
}
