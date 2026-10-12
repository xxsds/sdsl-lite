#include <array>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include <sdsl/construct.hpp>
#include <sdsl/csa_wt.hpp>
#include <sdsl/io.hpp>

#include <gtest/gtest.h>

namespace
{

std::string temp_dir;

TEST(IoTest, FormatLiteralsAndFields)
{
    sdsl::csa_wt<> csa;
    sdsl::construct_im(csa, std::string("banana"), 1);
    std::array<unsigned, 7> const sa = {6, 5, 3, 1, 0, 4, 2};
    std::string const bwt = "annb$aa";
    std::string const literal = std::string("a\0b", 3) + std::string(256, 'x');
    std::ostringstream actual, expected;
    sdsl::csXprintf(actual, literal + "%2I|%2S|%B|%%|tail%", csa);
    for (size_t i = 0; i < sa.size(); ++i)
        expected << literal << std::setw(2) << i << '|' << std::setw(2) << sa[i]
                 << '|' << bwt[i] << "|%|tail\n";
    EXPECT_EQ(expected.str(), actual.str());

    actual.str("");
    sdsl::csXprintf(actual, "plain text", csa);
    EXPECT_EQ(std::string("plain text\nplain text\nplain text\nplain text\n"
                          "plain text\nplain text\nplain text\n"), actual.str());
    actual.str("");
    sdsl::csXprintf(actual, "ignored%I", sdsl::csa_wt<>{});
    EXPECT_TRUE(actual.str().empty());
}

TEST(IoTest, LoadByteWidthsAcrossReadBoundary)
{
    std::string const file = temp_dir + "/io_plain_integers";
    for (uint8_t bytes : {1, 2, 4, 8})
    {
        for (uint64_t size : {uint64_t(0), uint64_t(7), sdsl::conf::SDSL_BLOCK_SIZE + 1})
        {
            SCOPED_TRACE("bytes=" + std::to_string(bytes) + ", size=" + std::to_string(size));
            uint8_t const width = 8 * bytes;
            sdsl::int_vector<> expected(size, 0, width), actual(size, 0, width);
            std::vector<char> data(size * bytes);
            for (uint64_t i = 0; i < size; ++i)
            {
                uint64_t value = i * 0x9e3779b97f4a7c15ULL;
                expected[i] = value;
                for (uint8_t j = 0; j < bytes; ++j)
                    data[i * bytes + j] = static_cast<char>(value >> (8 * j));
            }
            {
                sdsl::osfstream out(file, std::ios::binary | std::ios::trunc);
                out.write(data.data(), data.size());
                ASSERT_TRUE(out);
            }
            ASSERT_TRUE(sdsl::load_vector_from_file(actual, file, bytes));
            EXPECT_EQ(expected, actual);
            std::ostringstream expected_bytes, actual_bytes;
            expected.serialize(expected_bytes);
            actual.serialize(actual_bytes);
            EXPECT_EQ(expected_bytes.str(), actual_bytes.str());
        }
    }
    sdsl::remove(file);
}

TEST(IoTest, RamFilePrefix)
{
    EXPECT_FALSE(sdsl::is_ram_file(std::string{}));
    EXPECT_TRUE(sdsl::is_ram_file(std::string("@")));
    EXPECT_TRUE(sdsl::is_ram_file(std::string("@name")));
    EXPECT_FALSE(sdsl::is_ram_file(std::string("name")));
    EXPECT_FALSE(sdsl::is_ram_file(std::string("path/@name")));
}

} // namespace

int main(int argc, char ** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    if (argc < 2)
        return 1;
    temp_dir = argv[1];
    return RUN_ALL_TESTS();
}
