#include <filesystem>
#include <iterator>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

#include <sdsl/sfstream.hpp>
#include <sdsl/util.hpp>

#include <gtest/gtest.h>

namespace
{
static_assert(std::is_swappable_v<sdsl::isfstream>);
static_assert(std::is_swappable_v<sdsl::osfstream>);

class sfstream_swap_test : public ::testing::Test
{
protected:
    std::string disk;
    std::string ram;
    void SetUp() override
    {
        std::string name = "sdsl-swap-" + std::to_string(sdsl::util::pid()) + "-" + std::to_string(sdsl::util::id());
        disk = (std::filesystem::temp_directory_path() / name).string();
        ram = "@" + name;
        sdsl::osfstream(disk) << "abc";
        sdsl::osfstream(ram) << "xyz";
    }
    void TearDown() override
    {
        sdsl::remove(disk);
        sdsl::remove(ram);
    }
    std::string contents(std::string const & file)
    {
        sdsl::isfstream in(file);
        return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
    }
};

TEST_F(sfstream_swap_test, input_state_and_ownership)
{
    sdsl::isfstream left(disk), right(ram);
    auto * left_buffer = left.rdbuf();
    auto * right_buffer = right.rdbuf();
    char data[2];
    left.get();
    right.read(data, 2);
    left.setf(std::ios::hex, std::ios::basefield);
    left.fill('!');
    int const slot = std::ios::xalloc();
    left.iword(slot) = 17;
    left.setstate(std::ios::eofbit);
    left.exceptions(std::ios::badbit);
    right.exceptions(std::ios::failbit);
    std::ostringstream tied;
    left.tie(&tied);

    using std::swap;
    swap(left, right);
    EXPECT_EQ(right_buffer, left.rdbuf());
    EXPECT_EQ(left_buffer, right.rdbuf());
    EXPECT_TRUE(left.is_open());
    EXPECT_TRUE(right.is_open());
    EXPECT_TRUE(left.good());
    EXPECT_TRUE(right.eof());
    EXPECT_EQ(std::ios::failbit, left.exceptions());
    EXPECT_EQ(std::ios::badbit, right.exceptions());
    EXPECT_EQ(2, left.gcount());
    EXPECT_EQ(1, right.gcount());
    EXPECT_EQ(std::ios::hex, right.flags() & std::ios::basefield);
    EXPECT_EQ('!', right.fill());
    EXPECT_EQ(17, right.iword(slot));
    EXPECT_EQ(&tied, right.tie());
    right.clear();
    EXPECT_EQ('z', left.get());
    EXPECT_EQ('b', right.get());
    right.swap(right);
    EXPECT_EQ('c', right.get());
    right.tie(nullptr);
}

TEST_F(sfstream_swap_test, output_pending_data_and_format)
{
    {
        sdsl::osfstream left(disk), right(ram);
        auto * left_buffer = left.rdbuf();
        auto * right_buffer = right.rdbuf();
        left << std::hex << 'A';
        right << 'B';
        using std::swap;
        swap(left, right);
        EXPECT_EQ(right_buffer, left.rdbuf());
        EXPECT_EQ(left_buffer, right.rdbuf());
        EXPECT_TRUE(left.is_open());
        EXPECT_TRUE(right.is_open());
        left << 16;
        right << 16;
        right.swap(right);
        right << '!';
        left.close();
        right.close();
    }
    EXPECT_EQ("A10!", contents(disk));
    EXPECT_EQ("B16", contents(ram));
}

TEST_F(sfstream_swap_test, swaps_with_unopened_streams)
{
    for (std::string const & file : {disk, ram})
    {
        sdsl::isfstream reader(file), empty_reader;
        reader.get();
        empty_reader.swap(reader);
        EXPECT_FALSE(reader.is_open());
        EXPECT_EQ(nullptr, reader.rdbuf());
        EXPECT_TRUE(empty_reader.good());
        EXPECT_EQ(file == disk ? 'b' : 'y', empty_reader.get());
        sdsl::osfstream writer(file), empty_writer;
        writer << '1';
        empty_writer.swap(writer);
        EXPECT_FALSE(writer.is_open());
        EXPECT_EQ(nullptr, writer.rdbuf());
        EXPECT_TRUE(empty_writer.good());
        empty_writer << '2';
        empty_writer.close();
        EXPECT_EQ("12", contents(file));
    }
}
} // namespace
