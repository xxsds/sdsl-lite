#include "common.hpp"
#include <sdsl/suffix_arrays.hpp>
#include <sdsl/construct_sa.hpp>
#include <sdsl/construct_sa_se.hpp>
#include "gtest/gtest.h"
#include <vector>
#include <string>
#include <map>

using namespace sdsl;
using namespace std;

namespace
{
uint64_t n;
cache_config config;

class sa_construct_test : public ::testing::Test
{
    public:

    protected:
        sa_construct_test() { }

        virtual ~sa_construct_test() { }

        // If the constructor and destructor are not enough for setting up
        // and cleaning up each test, you can define the following methods:
        virtual void SetUp() {}

        virtual void TearDown() {}
};


TEST_F(sa_construct_test, divsufsort)
{
    // Construct SA with divsufsort
    memory_monitor::start();
    construct_config::byte_algo_sa = LIBDIVSUFSORT;
    construct_sa<8>(config);
    memory_monitor::stop();
    {
        int_vector_buffer<> sa_buf(cache_file_name(conf::KEY_SA, config));
        ASSERT_EQ(n, (uint64_t)sa_buf.size());
    }
    // Rename sa-file
    sdsl::rename(cache_file_name(conf::KEY_SA, config), cache_file_name("check_sa", config));
    config.file_map.erase(conf::KEY_SA);
    register_cache_file("check_sa", config);
    cout << "# constructs_space = " << (1.0*memory_monitor::peak())/n 
         << " byte per byte, =>" << memory_monitor::peak() 
         << " bytes in total" << endl;
}

TEST_F(sa_construct_test, sesais)
{
    // Construct SA with seSAIS
    memory_monitor::start();
    construct_config::byte_algo_sa = SE_SAIS;
    construct_sa<8>(config);
    memory_monitor::stop();
    {
        int_vector_buffer<> sa_buf(cache_file_name(conf::KEY_SA, config));
        ASSERT_EQ(n, (uint64_t)sa_buf.size());
    }
    cout << "# constructs_space = " << (1.0*memory_monitor::peak())/n 
         << " byte per byte, =>" << memory_monitor::peak() 
         << " bytes in total" << endl;
}

TEST_F(sa_construct_test, compare)
{
    // Load both SAs
    {
        int_vector_buffer<> sa_check(cache_file_name("check_sa", config));
        int_vector_buffer<> sa(cache_file_name(conf::KEY_SA, config));

        // Verify
        ASSERT_EQ(sa_check.size(), sa.size()) << " suffix array size differ";
        for (uint64_t i=0; i<sa_check.size(); ++i) {
            ASSERT_EQ(sa_check[i], sa[i]) << " sa differs at position " << i;
        }
    }
    // Remove all files
    util::delete_all_files(config.file_map);
}

}  // namespace

int main(int argc, char** argv)
{
    string test_file;
    string temp_dir;
    string temp_file;
    ::testing::InitGoogleTest(&argc, argv);
     if ( init_2_arg_test(argc, argv, "SA_CONSTRUCT", test_file, temp_dir, temp_file) != 0 ) {
        return 1;
    }
    string test_id     = to_string(util::pid());
    config = cache_config(false, temp_dir, test_id);
    if (!cache_file_exists(conf::KEY_TEXT, config)) {
        int_vector<8> text;
        load_vector_from_file(text, test_file, 1);
        if (contains_no_zero_symbol(text, test_file)) {
            append_zero_symbol(text);
            store_to_cache(text, conf::KEY_TEXT, config);
        }
        n = text.size();
    }
    register_cache_file(conf::KEY_TEXT, config);

    return RUN_ALL_TESTS();
}
