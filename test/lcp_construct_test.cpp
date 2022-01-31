#include <map>
#include <string>
#include <vector>

#include "sdsl/construct.hpp"
#include "sdsl/suffix_arrays.hpp"

#include "common.hpp"

#include <gtest/gtest.h>

using namespace sdsl;
using namespace std;

namespace
{

string test_file, temp_dir, temp_file;
typedef map<string, void (*)(cache_config &)> tMSFP; // map <name, lcp method>

class lcp_construct_test : public ::testing::Test
{
  protected:
    lcp_construct_test()
      : CHECK_KEY("CHECK_" + string(conf::KEY_LCP))
    {}

    virtual ~lcp_construct_test() {}

    // If the constructor and destructor are not enough for setting up
    // and cleaning up each test, you can define the following methods:
    virtual void SetUp()
    {
        test_config = cache_config(false, temp_dir, to_string(util::pid()));
        lcp_function["PHI"] = &construct_lcp_PHI<8>;
        lcp_function["semi_extern_PHI"] = &construct_lcp_semi_extern_PHI;
        lcp_function["go"] = &construct_lcp_go;
        lcp_function["goPHI"] = &construct_lcp_goPHI;
        lcp_function["bwt_based"] = &construct_lcp_bwt_based<>;
        lcp_function["bwt_based2"] = &construct_lcp_bwt_based2<>;

        uint8_t num_bytes = 1;
        {
            // Prepare Input
            int_vector<8> text;
            ASSERT_TRUE(load_vector_from_file(text, test_file, num_bytes));
            ASSERT_TRUE(contains_no_zero_symbol(text, test_file));
            append_zero_symbol(text);
            ASSERT_TRUE(store_to_cache(text, conf::KEY_TEXT, test_config));
            // Construct SA
            int_vector<> sa(text.size(), 0, bits::hi(text.size()) + 1);
            algorithm::calculate_sa((const unsigned char *)text.data(), text.size(), sa);
            ASSERT_TRUE(store_to_cache(sa, conf::KEY_SA, test_config));
        }
        {
            // Construct BWT
            construct_bwt<8>(test_config);
        }
        {
            // Construct LCP
            construct_lcp_kasai<8>(test_config);
            sdsl::rename(cache_file_name(conf::KEY_LCP, test_config), cache_file_name(CHECK_KEY, test_config));
            test_config.file_map.erase(conf::KEY_LCP);
            int_vector<> lcp;
            load_from_file(lcp, cache_file_name(CHECK_KEY, test_config));
        }
    }

    virtual void TearDown() { sdsl::remove(cache_file_name(CHECK_KEY, test_config)); }

    cache_config test_config;
    tMSFP lcp_function;
    string CHECK_KEY;
};

TEST_F(lcp_construct_test, construct_lcp)
{
    for (tMSFP::const_iterator it = this->lcp_function.begin(), end = this->lcp_function.end(); it != end; ++it)
    {
        string info = "construct_lcp_" + (it->first) + " on test file " + test_file;
        // Construct LCP array
        (it->second)(this->test_config);
        // Check LCP array
        int_vector<> lcp_check, lcp;
        string lcp_check_file = cache_file_name(CHECK_KEY, this->test_config);
        string lcp_file = cache_file_name(conf::KEY_LCP, this->test_config);
        ASSERT_TRUE(load_from_file(lcp_check, lcp_check_file)) << info << " could not load reference lcp array "
                                                               << lcp_check_file;
        ASSERT_TRUE(load_from_file(lcp, lcp_file)) << info << " could not load created lcp array";
        ASSERT_EQ(lcp_check.size(), lcp.size()) << info << " lcp array size differ";
        for (uint64_t j = 0; j < lcp.size(); ++j)
        {
            ASSERT_EQ(lcp_check[j], lcp[j]) << info << " value differ:"
                                            << " lcp_check[" << j << "]=" << lcp_check[j] << "!=" << lcp[j] << "=lcp["
                                            << j << "]";
        }
        // Clean up LCP array
        sdsl::remove(cache_file_name(conf::KEY_LCP, this->test_config));
    }
}

} // namespace

int main(int argc, char ** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    if (init_2_arg_test(argc, argv, "LCP_CONSTRUCT", test_file, temp_dir, temp_file) != 0) { return 1; }
    return RUN_ALL_TESTS();
}
