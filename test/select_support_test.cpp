#include <array>
#include <bit>
#include <cstring>
#include <random>
#include <span>
#include <string>
#include <vector>

#include <sdsl/bit_vector_il.hpp>
#include <sdsl/hyb_vector.hpp>
#include <sdsl/rrr_vector.hpp>
#include <sdsl/sd_vector.hpp>
#include <sdsl/select_support_mcl.hpp>
#include <sdsl/select_support_scan.hpp>

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
class select_support_test : public ::testing::Test
{};

using testing::Types;

#ifdef FULL_TEST_SUITE

typedef Types<select_support_mcl<>,
              select_support_hyb<1>,
              select_support_hyb<0>,
              select_support_sd<1>,
              select_support_sd<0>,
              select_0_support_sd<>,
              select_support_il<1, 512>,
              select_support_mcl<0>,
              select_support_il<0, 512>,
              select_support_mcl<01, 2>,
              select_support_mcl<10, 2>,
              select_support_mcl<00, 2>,
              select_support_mcl<11, 2>,
              select_support_rrr<1, 256>,
              select_support_rrr<1, 129>,
              select_support_rrr<1, 192>,
              select_support_rrr<1, 255>,
              select_support_rrr<1, 15>,
              select_support_rrr<1, 31>,
              select_support_rrr<1, 63>,
              select_support_rrr<1, 127>,
              select_support_rrr<1, 128>,
              select_support_il<1, 256>,
              select_support_il<1, 1024>,
              select_support_rrr<0, 256>,
              select_support_rrr<0, 15>,
              select_support_rrr<0, 31>,
              select_support_rrr<0, 63>,
              select_support_rrr<0, 127>,
              select_support_il<0, 256>,
              select_support_il<0, 1024>>
    Implementations;

#else

typedef Types<select_support_mcl<>,
              select_support_hyb<1>,
              select_support_hyb<0>,
              select_support_rrr<>,
              select_support_sd<1>,
              select_support_sd<0>,
              select_0_support_sd<>,
              select_support_il<1, 512>,
              select_support_mcl<0>,
              select_support_rrr<0>,
              select_support_il<0, 512>,
              select_support_mcl<01, 2>,
              select_support_mcl<10, 2>,
              select_support_mcl<00, 2>,
              select_support_mcl<11, 2>>
    Implementations;

#endif

TYPED_TEST_SUITE(select_support_test, Implementations, );

TEST(plain_query_test, byte_and_word_storage)
{
    std::array<uint64_t, 4> words = {0, uint64_t{1} << 63, 0x5555555555555555ULL, ~uint64_t{0}};
    for (size_t offset = 0; offset < alignof(uint64_t); ++offset)
    {
        SCOPED_TRACE(offset);
        std::array<uint8_t, sizeof(words) + alignof(uint64_t) - 1> bytes;
        std::memcpy(bytes.data() + offset, words.data(), sizeof(words));
        auto byte_data = std::span<uint8_t const>(bytes.data() + offset, sizeof(words));
        uint64_t ones = 0, zeros = 0;
        for (uint64_t position = 0; position < 256; ++position)
        {
            ASSERT_EQ(ones, detail::rank_in_words(std::span(words), position));
            ASSERT_EQ(ones, detail::rank_in_words(byte_data, position));
            bool bit = (words[position / 64] >> (position % 64)) & 1;
            ASSERT_EQ(bit, detail::access_in_words(std::span(words), position));
            ASSERT_EQ(bit, detail::access_in_words(byte_data, position));
            if (bit)
            {
                ASSERT_EQ(position, detail::select_in_words<1>(std::span(words), ++ones));
                ASSERT_EQ(position, detail::select_in_words<1>(byte_data, ones));
            }
            else
            {
                ASSERT_EQ(position, detail::select_in_words<0>(std::span(words), ++zeros));
                ASSERT_EQ(position, detail::select_in_words<0>(byte_data, zeros));
            }
        }
        ASSERT_EQ(ones, detail::rank_in_words(std::span(words), 256));
        ASSERT_EQ(ones, detail::rank_in_words(byte_data, 256));
    }
}

TEST(plain_query_test, long_rank_prefixes)
{
    std::array<uint64_t, 17> words;
    for (size_t word = 0; word < words.size(); ++word)
        words[word] = 0x0123456789abcdefULL * (word + 1);
    constexpr uint64_t initial_rank = uint64_t{1} << 40;
    uint64_t expected = initial_rank;
    for (size_t prefix = 0; prefix <= 64 * words.size(); ++prefix)
    {
        auto data = std::span<uint64_t const>(words).first((prefix + 63) / 64);
        ASSERT_EQ(expected, detail::rank_in_words(data, prefix, initial_rank));
        if (prefix < 64 * words.size())
            expected += (words[prefix / 64] >> (prefix % 64)) & 1;
    }
    ASSERT_EQ(initial_rank, detail::rank_in_words(std::span<uint64_t const>{}, 0, initial_rank));
}

template <class T>
class cached_select_test : public ::testing::Test
{};

using CachedSelect = Types<select_support_il<0, 512>, select_support_il<1, 512>,
                           select_support_il<0, 4096>, select_support_il<1, 4096>>;
TYPED_TEST_SUITE(cached_select_test, CachedSelect, );

TYPED_TEST(cached_select_test, repeated_populations_and_partial_last_block)
{
    bit_vector bvec((1 << 23) + 17); // Large enough to populate the select cache.
    size_t words = (bvec.size() + 63) / 64;
    std::vector<uint64_t> prefixes(words + 1);
    std::mt19937_64 random(42);
    for (size_t word = 0; word < words; ++word)
    {
        uint64_t value = random();
        if (word % 193 < 64)
            value = 0;
        else if (word % 193 < 128)
            value = ~uint64_t{0};
        value &= bits::lo_set[std::min<uint64_t>(64, bvec.size() - 64 * word)];
        bvec.data()[word] = value;
        prefixes[word + 1] = prefixes[word] + std::popcount(value);
    }
    typename TypeParam::bit_vector_type vector(bvec);
    TypeParam select(&vector);
    uint64_t total = TypeParam::bit_pat ? prefixes.back() : bvec.size() - prefixes.back();
    for (size_t query = 0; query < 2000; ++query)
    {
        uint64_t occurrence = query == 0 ? 1 : query == 1 ? total : random() % total + 1;
        uint64_t position = select(occurrence);
        ASSERT_LT(position, bvec.size());
        ASSERT_EQ(TypeParam::bit_pat, bvec[position]);
        uint64_t ones = prefixes[position / 64]
                      + std::popcount(bvec.data()[position / 64] & bits::lo_set[position % 64]);
        ASSERT_EQ(occurrence - 1, TypeParam::bit_pat ? ones : position - ones);
    }
}

//! Test the select method
TYPED_TEST(select_support_test, select_method)
{
    static_assert(sdsl::util::is_regular<TypeParam>::value, "Type is not regular");
    bit_vector bvec;
    ASSERT_TRUE(load_from_file(bvec, test_file));
    typename TypeParam::bit_vector_type bv(bvec);
    TypeParam ss(&bv);
    for (uint64_t j = 0, select = 0; j < bvec.size(); ++j)
    {
        bool found = (j >= TypeParam::bit_pat_len - 1);
        for (uint8_t k = 0; found and k < TypeParam::bit_pat_len; ++k)
        {
            found &= bvec[j - k] == ((TypeParam::bit_pat >> k) & 1);
        }
        if (found)
        {
            ++select;
            ASSERT_EQ(j, ss.select(select));
        }
    }
}

#if SDSL_HAS_CEREAL
template <typename in_archive_t, typename out_archive_t, typename TypeParam>
void do_serialisation(TypeParam const & l, typename TypeParam::bit_vector_type const & bv)
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
        in_l.set_vector(&bv);
        EXPECT_EQ(l, in_l);
    }
}

TYPED_TEST(select_support_test, cereal)
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
    if (init_2_arg_test(argc, argv, "SELECT_SUPPORT", test_file, temp_dir, temp_file) != 0)
    {
        return 1;
    }
    return RUN_ALL_TESTS();
}
