#include <algorithm>
#include <cstdint>
#include <functional>
#include <iostream>
#include <random>
#include <stack>
#include <string>
#include <type_traits>
#include <utility>

#include <sdsl/cereal.hpp>
#include <sdsl/io.hpp>
#include <sdsl/ram_fs.hpp>
#include <sdsl/sorted_int_stack.hpp>

#include <gtest/gtest.h> // IWYU pragma: keep
// IWYU pragma: no_include "gtest/gtest_pred_impl.h"
// IWYU pragma: no_include <gtest/gtest-message.h>
// IWYU pragma: no_include <gtest/gtest-test-part.h>
// IWYU pragma: no_include <gtest/gtest-typed-test.h>

namespace
{

std::string temp_dir;

// The fixture for testing class sorted_int_stack.
class sorted_stack_test : public ::testing::Test
{
  protected:
    sorted_stack_test() {}

    virtual ~sorted_stack_test() {}

    virtual void SetUp() {}

    virtual void TearDown() {}
};

void compare_stacks(std::stack<uint64_t> & exp, sdsl::sorted_int_stack & sis)
{
    ASSERT_EQ(exp.size(), sis.size());
    std::stack<uint64_t> tmp;
    while (!exp.empty())
    {
        ASSERT_EQ(exp.top(), sis.top());
        tmp.push(exp.top());
        exp.pop();
        sis.pop();
    }
    ASSERT_EQ(exp.size(), sis.size());
    // Restore stacks
    while (!tmp.empty())
    {
        exp.push(tmp.top());
        sis.push(tmp.top());
        tmp.pop();
    }
}

//! Test Constructors
TEST_F(sorted_stack_test, constructors)
{
    static_assert(std::is_copy_constructible<sdsl::sorted_int_stack>::value, "Type is not copy constructible");
    static_assert(std::is_move_constructible<sdsl::sorted_int_stack>::value, "Type is not move constructible");
    static_assert(std::is_copy_assignable<sdsl::sorted_int_stack>::value, "Type is not copy assignable");
    static_assert(std::is_move_assignable<sdsl::sorted_int_stack>::value, "Type is not move assignable");
    std::stack<uint64_t> exp;
    sdsl::sorted_int_stack sis1(100000 + 10);
    {
        std::mt19937_64 rng;
        std::uniform_int_distribution<uint64_t> distribution(0, 10);
        auto dice = bind(distribution, rng);
        for (uint64_t k = 0; k < 100000; ++k)
        {
            uint64_t value = k + dice();
            if (exp.empty() or exp.top() < value)
            {
                exp.push(value);
                sis1.push(value);
            }
        }
    }

    // Copy-constructor
    sdsl::sorted_int_stack sis2(sis1);
    compare_stacks(exp, sis2);

    // Move-constructor
    sdsl::sorted_int_stack sis3(std::move(sis2));
    compare_stacks(exp, sis3);
    // ASSERT_EQ((uint64_t)0, sis2.size());

    // Copy-Assign
    sdsl::sorted_int_stack sis4(0);
    sis4 = sis3;
    compare_stacks(exp, sis4);

    // Move-Assign
    sdsl::sorted_int_stack sis5(0);
    sis5 = std::move(sis4);
    compare_stacks(exp, sis5);
    // ASSERT_EQ((uint64_t)0, sis4.size());
}

//! Test Operations push, top and pop
TEST_F(sorted_stack_test, push_top_and_pop)
{
    for (uint64_t i = 0; i < 20; ++i)
    {
        std::mt19937_64 rng(i);
        std::uniform_int_distribution<uint64_t> distribution(0, i * i);
        auto dice = bind(distribution, rng);
        std::stack<uint64_t> exp;
        sdsl::sorted_int_stack sis(900000 + i * i);
        ASSERT_TRUE(sis.empty());
        for (uint64_t k = 0; k < 1000000; ++k)
        {
            ASSERT_EQ(exp.size(), sis.size());
            uint64_t value = k + dice();
            if (exp.empty())
            {
                exp.push(value);
                sis.push(value);
            }
            else
            {
                ASSERT_EQ(exp.top(), sis.top());
                if (exp.top() >= value)
                {
                    exp.pop();
                    sis.pop();
                }
                else
                {
                    exp.push(value);
                    sis.push(value);
                }
            }
        }
    }
}

//! Test Serialize and Load
TEST_F(sorted_stack_test, serialize_and_load)
{
    std::string file_name = temp_dir + "/sorted_int_stack";
    std::stack<uint64_t> exp;
    {
        std::mt19937_64 rng;
        std::uniform_int_distribution<uint64_t> distribution(0, 10);
        auto dice = bind(distribution, rng);
        sdsl::sorted_int_stack sis(1000000 + 10);
        for (uint64_t k = 0; k < 1000000; ++k)
        {
            uint64_t value = k + dice();
            if (exp.empty() or exp.top() < value)
            {
                exp.push(value);
                sis.push(value);
            }
        }
        sdsl::store_to_file(sis, file_name);
    }
    {
        sdsl::sorted_int_stack sis(0);
        sdsl::load_from_file(sis, file_name);
        compare_stacks(exp, sis);
    }
    sdsl::remove(file_name);
}

#if SDSL_HAS_CEREAL
template <typename in_archive_t, typename out_archive_t>
void do_serialisation(sdsl::sorted_int_stack const & l, std::string const & temp_file)
{
    {
        std::ofstream os{ temp_file, std::ios::binary };
        out_archive_t oarchive{ os };
        oarchive(l);
    }

    {
        sdsl::sorted_int_stack in_l(1000000 + 10);
        std::ifstream is{ temp_file, std::ios::binary };
        in_archive_t iarchive{ is };
        iarchive(in_l);
        EXPECT_EQ(l, in_l);
    }
}

TEST_F(sorted_stack_test, cereal)
{
    if (temp_dir != "@/")
    {
        std::string file_name = temp_dir + "/sorted_int_stack";
        sdsl::sorted_int_stack sis(1000000 + 10);
        std::stack<uint64_t> exp;
        {
            std::mt19937_64 rng;
            std::uniform_int_distribution<uint64_t> distribution(0, 10);
            auto dice = bind(distribution, rng);
            for (uint64_t k = 0; k < 1000000; ++k)
            {
                uint64_t value = k + dice();
                if (exp.empty() or exp.top() < value)
                {
                    exp.push(value);
                    sis.push(value);
                }
            }
            sdsl::store_to_file(sis, file_name);
        }

        do_serialisation<cereal::BinaryInputArchive, cereal::BinaryOutputArchive>(sis, file_name);
        do_serialisation<cereal::PortableBinaryInputArchive, cereal::PortableBinaryOutputArchive>(sis, file_name);
        do_serialisation<cereal::JSONInputArchive, cereal::JSONOutputArchive>(sis, file_name);
        do_serialisation<cereal::XMLInputArchive, cereal::XMLOutputArchive>(sis, file_name);

        sdsl::remove(file_name);
    }
}
#endif // SDSL_HAS_CEREAL

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
    return RUN_ALL_TESTS();
}
