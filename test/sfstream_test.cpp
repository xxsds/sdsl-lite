#include <filesystem>
#include <limits>
#include <string>

#include <sdsl/sfstream.hpp>
#include <sdsl/util.hpp>

#include <gtest/gtest.h>

namespace
{
class sfstream_test : public ::testing::TestWithParam<bool>
{
protected:
    std::string file;
    void SetUp() override
    {
        std::string name = "sdsl-stream-" + std::to_string(sdsl::util::pid()) + "-" + std::to_string(sdsl::util::id());
        file = GetParam() ? "@" + name : (std::filesystem::temp_directory_path() / name).string();
        sdsl::osfstream out(file);
        out << "abc";
        ASSERT_TRUE(out.good());
    }
    void TearDown() override
    {
        sdsl::remove(file);
    }
};

TEST_P(sfstream_test, input_positions)
{
    sdsl::isfstream in(file);
    EXPECT_EQ(0, in.tellg());
    in.seekg(1);
    EXPECT_EQ('b', in.get());
    in.seekg(-1, std::ios::cur);
    EXPECT_EQ('b', in.get());
    in.seekg(-1, std::ios::end);
    EXPECT_EQ('c', in.get());
    EXPECT_EQ(3, in.tellg());
}

TEST_P(sfstream_test, seek_after_eof)
{
    for (bool relative : {false, true})
    {
        sdsl::isfstream in(file);
        in.seekg(3);
        EXPECT_EQ(std::char_traits<char>::eof(), in.peek());
        ASSERT_TRUE(in.eof());
        if (relative)
            in.seekg(0, std::ios::beg);
        else
            in.seekg(0);
        EXPECT_TRUE(in.good());
        EXPECT_EQ('a', in.get());
    }
}

TEST_P(sfstream_test, invalid_input_seek)
{
    for (bool relative : {false, true})
    {
        sdsl::isfstream in(file);
        if (relative)
            in.seekg(-1, std::ios::beg);
        else
            in.seekg(-1);
        EXPECT_TRUE(in.fail());
        in.clear();
        in.exceptions(std::ios::failbit);
        if (relative)
            EXPECT_THROW(in.seekg(-1, std::ios::beg), std::ios_base::failure);
        else
            EXPECT_THROW(in.seekg(-1), std::ios_base::failure);
    }
}

TEST_P(sfstream_test, output_positions)
{
    sdsl::osfstream out(file, std::ios::in | std::ios::out);
    EXPECT_EQ(0, out.tellp());
    out.seekp(1);
    out.put('X');
    EXPECT_EQ(2, out.tellp());
    out.seekp(-1, std::ios::cur);
    out.put('Y');
    out.seekp(-1, std::ios::end);
    out.put('Z');
    out.close();
    sdsl::isfstream in(file);
    std::string result(3, '\0');
    in.read(result.data(), result.size());
    EXPECT_EQ("aYZ", result);
}

TEST_P(sfstream_test, invalid_output_seek)
{
    for (bool relative : {false, true})
    {
        sdsl::osfstream out(file, std::ios::in | std::ios::out);
        if (relative)
            out.seekp(-1, std::ios::beg);
        else
            out.seekp(-1);
        EXPECT_TRUE(out.fail());
        out.clear();
        out.exceptions(std::ios::failbit);
        if (relative)
            EXPECT_THROW(out.seekp(-1, std::ios::beg), std::ios_base::failure);
        else
            EXPECT_THROW(out.seekp(-1), std::ios_base::failure);
    }
}

INSTANTIATE_TEST_SUITE_P(disk_and_ram, sfstream_test, ::testing::Bool());

TEST(ram_filebuf_test, independent_positions_and_invalid_offsets)
{
    sdsl::ram_fs::content_type data{'a', 'b', 'c'};
    sdsl::ram_filebuf buffer(data);
    std::istream in(&buffer);
    std::ostream out(&buffer);
    in.seekg(1);
    out.seekp(2);
    EXPECT_EQ(1, in.tellg());
    EXPECT_EQ(2, out.tellp());
    out.put('Z');
    EXPECT_EQ('b', in.get());
    EXPECT_EQ('Z', in.get());
    in.seekg(std::numeric_limits<std::streamoff>::max(), std::ios::cur);
    EXPECT_TRUE(in.fail());
    EXPECT_EQ(3u, data.size());
    in.clear();
    in.seekg(4);
    EXPECT_TRUE(in.fail());
    in.clear();
    out.seekp(5);
    out.put('X');
    EXPECT_EQ(6, out.tellp());
    ASSERT_EQ(6u, data.size());
    EXPECT_EQ('\0', in.get());
    EXPECT_EQ('\0', data[4]);
    EXPECT_EQ('X', data[5]);
    buffer.close();
    EXPECT_EQ(-1, buffer.pubseekpos(0));
    EXPECT_EQ(-1, buffer.pubseekoff(0, std::ios::beg));
}
} // namespace
