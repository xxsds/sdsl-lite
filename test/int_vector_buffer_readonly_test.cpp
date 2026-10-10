#include <filesystem>
#include <fstream>
#include <string>
#include <utility>

#include <sdsl/int_vector_buffer.hpp>
#include <sdsl/io.hpp>

#include <gtest/gtest.h>

namespace
{
class readonly_buffer_test : public ::testing::TestWithParam<bool>
{
protected:
    std::string file;
    void SetUp() override
    {
        std::string name =
            "sdsl-readonly-" + std::to_string(sdsl::util::pid()) + "-" + std::to_string(sdsl::util::id());
        file = GetParam() ? "@" + name : (std::filesystem::temp_directory_path() / name).string();
        sdsl::int_vector<> data(100, 0, 7);
        for (size_t i = 0; i < data.size(); ++i)
            data[i] = i;
        ASSERT_TRUE(sdsl::store_to_file(data, file));
    }
    void TearDown() override
    {
        if (!GetParam())
            std::filesystem::permissions(file, std::filesystem::perms::owner_all);
        sdsl::remove(file);
    }
};

TEST_P(readonly_buffer_test, reads_across_blocks_and_rejects_writes)
{
    sdsl::int_vector_buffer<> buffer(file, std::ios::in, 8);
    ASSERT_TRUE(buffer.is_open());
    ASSERT_TRUE(buffer.good());
    for (size_t i = 0; i < buffer.size(); ++i)
        EXPECT_EQ(i, uint64_t(buffer[i]));
    buffer.buffersize(16);
    EXPECT_EQ(0u, uint64_t(buffer[0]));
    EXPECT_THROW(buffer[0] = 10, std::ios_base::failure);
    EXPECT_THROW(buffer.push_back(10), std::ios_base::failure);
    EXPECT_THROW(buffer.reset(), std::ios_base::failure);
    buffer.close();
    EXPECT_FALSE(buffer.is_open());
    sdsl::int_vector<> restored;
    ASSERT_TRUE(sdsl::load_from_file(restored, file));
    ASSERT_EQ(100u, restored.size());
    for (size_t i = 0; i < restored.size(); ++i)
        EXPECT_EQ(i, restored[i]);
}

TEST_P(readonly_buffer_test, moves_preserve_read_only_mode)
{
    sdsl::int_vector_buffer<> source(file, std::ios::in, 8);
    sdsl::int_vector_buffer<> moved(std::move(source));
    EXPECT_FALSE(source.is_open());
    EXPECT_TRUE(moved.good());
    EXPECT_EQ(99u, uint64_t(moved[99]));
    EXPECT_THROW(moved[0] = 10, std::ios_base::failure);
    sdsl::int_vector_buffer<> assigned;
    assigned = std::move(moved);
    EXPECT_FALSE(moved.is_open());
    EXPECT_TRUE(assigned.good());
    EXPECT_EQ(0u, uint64_t(assigned[0]));
    EXPECT_THROW(assigned.reset(), std::ios_base::failure);
    sdsl::int_vector_buffer<> empty(std::move(source));
    EXPECT_FALSE(empty.is_open());
}

TEST_P(readonly_buffer_test, explicit_read_write_and_reset)
{
    {
        sdsl::int_vector_buffer<> buffer(file, std::ios::in | std::ios::out, 8);
        buffer[1] = 42;
        sdsl::int_vector_buffer<> moved(std::move(buffer));
        sdsl::int_vector_buffer<> assigned;
        assigned = std::move(moved);
        EXPECT_EQ(42u, uint64_t(assigned[1]));
    }
    {
        sdsl::int_vector_buffer<> buffer(file);
        EXPECT_EQ(42u, uint64_t(buffer[1]));
    }
    {
        sdsl::int_vector_buffer<> buffer(file, std::ios::in | std::ios::out);
        buffer.reset();
        buffer.push_back(7);
    }
    sdsl::int_vector_buffer<> buffer(file);
    EXPECT_EQ(1u, buffer.size());
    EXPECT_EQ(7u, uint64_t(buffer[0]));
}

TEST_P(readonly_buffer_test, move_assignment_closes_previous_destination)
{
    std::string const previous = file + ".previous";
    {
        sdsl::int_vector_buffer<> destination(previous, std::ios::out, 8, 7);
        destination.push_back(19); // still buffered when ownership changes
        sdsl::int_vector_buffer<> source(file, std::ios::in | std::ios::out, 8);
        source[1] = 42;
        destination = std::move(source);
        EXPECT_FALSE(source.is_open());
        EXPECT_TRUE(source.filename().empty());
        EXPECT_EQ(0u, source.size());
        EXPECT_EQ(64u, source.width());
        sdsl::int_vector<> saved;
        ASSERT_TRUE(sdsl::load_from_file(saved, previous));
        ASSERT_EQ(1u, saved.size());
        EXPECT_EQ(19u, saved[0]);
        EXPECT_EQ(42u, uint64_t(destination[1]));
        auto & alias = destination;
        destination = std::move(alias);
        EXPECT_EQ(42u, uint64_t(destination[1]));
    }
    sdsl::int_vector<> saved;
    ASSERT_TRUE(sdsl::load_from_file(saved, file));
    EXPECT_EQ(42u, saved[1]);
    sdsl::remove(previous);
}

TEST_P(readonly_buffer_test, move_keeps_open_file)
{
    if (GetParam())
        GTEST_SKIP() << "This test uses a filesystem rename";
    sdsl::int_vector_buffer<> source(file, std::ios::in | std::ios::out, 8);
    source[1] = 42;
    std::string const renamed = file + ".renamed";
    std::error_code error;
    std::filesystem::rename(file, renamed, error);
    if (error)
        GTEST_SKIP() << "This filesystem cannot rename the open file: " << error.message();
    file = renamed;
    {
        sdsl::int_vector_buffer<> moved(std::move(source));
        sdsl::int_vector_buffer<> assigned;
        assigned = std::move(moved);
        EXPECT_TRUE(assigned.good());
        EXPECT_EQ(42u, uint64_t(assigned[1]));
        // A different block exercises the transferred input stream too.
        EXPECT_EQ(99u, uint64_t(assigned[99]));
    }
    sdsl::int_vector<> saved;
    ASSERT_TRUE(sdsl::load_from_file(saved, file));
    EXPECT_EQ(42u, saved[1]);
}

TEST_P(readonly_buffer_test, reads_file_without_write_permission)
{
    if (GetParam())
        GTEST_SKIP() << "RAM files have no permission bits";
    std::filesystem::permissions(file, std::filesystem::perms::owner_read);
    std::ofstream check(file, std::ios::in | std::ios::out | std::ios::binary);
    if (check.is_open())
        GTEST_SKIP() << "This environment allows writes despite the file permissions";
    sdsl::int_vector_buffer<> buffer(file);
    EXPECT_TRUE(buffer.good());
    EXPECT_EQ(99u, uint64_t(buffer[99]));
}

INSTANTIATE_TEST_SUITE_P(disk_and_ram, readonly_buffer_test, ::testing::Bool());
} // namespace
