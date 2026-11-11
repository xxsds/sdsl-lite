#include <algorithm>
#include <fcntl.h>
#include <random>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>

#include <sdsl/int_vector_mapper.hpp>
#include <sdsl/util.hpp>

#include <gtest/gtest.h>

namespace
{

typedef sdsl::int_vector<>::size_type size_type;
typedef sdsl::int_vector<>::value_type value_type;

std::string temp_dir;

// The fixture for testing class int_vector.
class int_vector_mapper_test : public ::testing::Test
{
protected:
    int_vector_mapper_test()
    {}

    virtual ~int_vector_mapper_test()
    {}

    virtual void SetUp()
    {
        std::mt19937_64 rng;
        {
            std::uniform_int_distribution<uint64_t> distribution(1, 100000);
            auto dice = bind(distribution, rng);
            for (size_type i = 0; i < 10; ++i)
            {
                vec_sizes.push_back(dice());
            }
        }
    }

    virtual void TearDown()
    {}

    std::vector<size_type> vec_sizes = {1, 64, 65, 127, 128}; // different sizes for the vectors
};

TEST_F(int_vector_mapper_test, iterator)
{
    static_assert(std::is_move_constructible<sdsl::int_vector_mapper<>>::value, "Type is not move constructible");
    static_assert(std::is_move_assignable<sdsl::int_vector_mapper<>>::value, "Type is not move assignable");

    // test plain
    for (auto const & size : vec_sizes)
    {
        std::vector<uint64_t> vec(size);
        sdsl::util::set_to_id(vec);
        {
            sdsl::osfstream ofs(temp_dir + "/int_vector_mapper_itrtest",
                                std::ios::binary | std::ios::trunc | std::ios::out);
            sdsl::serialize_vector(vec, ofs);
        }
        {
            sdsl::int_vector_mapper<64, std::ios_base::in> const ivm(temp_dir + "/int_vector_mapper_itrtest", true);
            ASSERT_EQ(size, ivm.size());
            ASSERT_TRUE(std::equal(ivm.begin(), ivm.end(), vec.begin()));
            ASSERT_EQ(size, (size_t)std::distance(ivm.begin(), ivm.end()));
        }
        {
            sdsl::int_vector_mapper<64, std::ios_base::in> const ivm(temp_dir + "/int_vector_mapper_itrtest", true);
            auto itr = ivm.end() - 1;
            for (size_t i = 0; i < size; i++)
            {
                ASSERT_EQ(*itr, vec[size - i - 1]);
                --itr;
            }
        }
        sdsl::remove(temp_dir + "/int_vector_mapper_itrtest");
    }

    // test fixed width
    for (auto const & size : vec_sizes)
    {
        sdsl::int_vector<25> vec(size);
        sdsl::util::set_to_id(vec);
        store_to_file(vec, temp_dir + "/int_vector_mapper_itrtest");
        {
            sdsl::int_vector_mapper<25, std::ios_base::in> const ivm(temp_dir + "/int_vector_mapper_itrtest");
            ASSERT_EQ(size, ivm.size());
            ASSERT_TRUE(std::equal(ivm.begin(), ivm.end(), vec.begin()));
            ASSERT_EQ(size, (size_t)std::distance(ivm.begin(), ivm.end()));
        }
        {
            sdsl::int_vector_mapper<25, std::ios_base::in> const ivm(temp_dir + "/int_vector_mapper_itrtest");
            auto itr = ivm.end() - 1;
            for (size_t i = 0; i < size; i++)
            {
                ASSERT_EQ(*itr, vec[size - i - 1]);
                --itr;
            }
        }
        sdsl::remove(temp_dir + "/int_vector_mapper_itrtest");
    }

    // test variable width
    for (auto const & size : vec_sizes)
    {
        sdsl::int_vector<> vec(size);
        sdsl::util::set_to_id(vec);
        sdsl::util::bit_compress(vec);
        store_to_file(vec, temp_dir + "/int_vector_mapper_itrtest");
        {
            sdsl::int_vector_mapper<0, std::ios_base::in> const ivm(temp_dir + "/int_vector_mapper_itrtest");
            ASSERT_EQ(size, ivm.size());
            ASSERT_EQ(vec.width(), ivm.width());
            ASSERT_TRUE(std::equal(ivm.begin(), ivm.end(), vec.begin()));
            ASSERT_EQ(size, (size_t)std::distance(ivm.begin(), ivm.end()));
        }
        {
            sdsl::int_vector_mapper<> ivm(temp_dir + "/int_vector_mapper_itrtest");
            auto itr = ivm.end() - 1;
            for (size_t i = 0; i < size; i++)
            {
                ASSERT_EQ(*itr, vec[size - i - 1]);
                --itr;
            }
        }
        sdsl::remove(temp_dir + "/int_vector_mapper_itrtest");
    }
}

template <uint8_t width>
void check_move_constructor(std::string const & filename)
{
    sdsl::int_vector<width> expected(4, 0, 17);
    sdsl::util::set_to_id(expected);
    ASSERT_TRUE(sdsl::store_to_file(expected, filename));
    {
        sdsl::int_vector_mapper<width> source(filename, false, true);
        auto const capacity = source.capacity();
        sdsl::int_vector_mapper<width> moved(std::move(source));
        ASSERT_EQ(capacity, moved.capacity());
        ASSERT_EQ(expected.width(), moved.width());
        ASSERT_EQ(moved, expected);
        EXPECT_TRUE(source.empty());

        moved.push_back(7);
        EXPECT_EQ(5u, moved.size());
        EXPECT_EQ(7u, moved[4]);
    }
    EXPECT_FALSE(sdsl::isfstream(filename).is_open());
}

TEST_F(int_vector_mapper_test, move_constructor)
{
    check_move_constructor<0>(temp_dir + "/mapper_move_dynamic");
    check_move_constructor<31>(temp_dir + "/mapper_move_fixed");
}

template <uint8_t width>
void check_move_assignment(std::string const & filename)
{
    auto const replaced_filename = filename + "_replaced";
    sdsl::int_vector<width> expected(4, 0, 17);
    sdsl::util::set_to_id(expected);
    ASSERT_TRUE(sdsl::store_to_file(expected, filename));
    ASSERT_TRUE(sdsl::store_to_file(sdsl::int_vector<width>(2, 1, 9), replaced_filename));
    {
        sdsl::int_vector_mapper<width> source(filename, false, true);
        sdsl::int_vector_mapper<width> destination(replaced_filename, false, true);
        auto const capacity = source.capacity();
        destination = std::move(source);
        EXPECT_FALSE(sdsl::isfstream(replaced_filename).is_open());
        ASSERT_EQ(capacity, destination.capacity());
        ASSERT_EQ(expected.width(), destination.width());
        ASSERT_EQ(destination, expected);
        EXPECT_TRUE(source.empty());

        auto & alias = destination;
        destination = std::move(alias);
        ASSERT_EQ(destination, expected);
        destination.push_back(7);
        EXPECT_EQ(5u, destination.size());
        EXPECT_EQ(7u, destination[4]);

        source = std::move(destination);
        EXPECT_TRUE(destination.empty());
        EXPECT_EQ(5u, source.size());
        EXPECT_EQ(7u, source[4]);
    }
    EXPECT_FALSE(sdsl::isfstream(filename).is_open());
}

TEST_F(int_vector_mapper_test, move_assignment)
{
    check_move_assignment<0>(temp_dir + "/mapper_assign_dynamic");
    check_move_assignment<31>(temp_dir + "/mapper_assign_fixed");
}

TEST_F(int_vector_mapper_test, push_back)
{
    // test plain
    for (auto const & size : vec_sizes)
    {
        std::vector<uint64_t> vec(size);
        sdsl::util::set_to_id(vec);
        {
            sdsl::osfstream ofs(temp_dir + "/int_vector_mapper_push_backtest",
                                std::ios::binary | std::ios::trunc | std::ios::out);
            sdsl::serialize_vector(vec, ofs);
        }
        {
            sdsl::int_vector_mapper<64> ivm(temp_dir + "/int_vector_mapper_push_backtest", true);
            ASSERT_EQ(size, ivm.size());
            for (size_t i = 0; i < size; i++)
            {
                vec.push_back(vec.size());
                ivm.push_back(ivm.size());
            }
        }
        {
            sdsl::int_vector_mapper<64> ivm(temp_dir + "/int_vector_mapper_push_backtest", true);
            ASSERT_EQ(vec.size(), ivm.size());
            ASSERT_TRUE(std::equal(ivm.begin(), ivm.end(), vec.begin()));
            ASSERT_EQ(vec.size(), (size_t)std::distance(ivm.begin(), ivm.end()));
        }
        sdsl::remove(temp_dir + "/int_vector_mapper_itrtest");
    }

    // test fixed width
    for (auto const & size : vec_sizes)
    {
        sdsl::int_vector<31> vec(size);
        std::vector<uint64_t> stdvec(size);
        sdsl::util::set_to_id(vec);
        sdsl::util::set_to_id(stdvec);
        store_to_file(vec, temp_dir + "/int_vector_mapper_push_backtest");
        {
            sdsl::int_vector_mapper<31> ivm(temp_dir + "/int_vector_mapper_push_backtest");
            ASSERT_EQ(size, ivm.size());
            for (size_t i = 0; i < size; i++)
            {
                stdvec.push_back(stdvec.size());
                ivm.push_back(ivm.size());
            }
        }
        {
            sdsl::int_vector_mapper<31> ivm(temp_dir + "/int_vector_mapper_push_backtest");
            ASSERT_EQ(stdvec.size(), ivm.size());
            ASSERT_TRUE(std::equal(ivm.begin(), ivm.end(), stdvec.begin()));
            ASSERT_EQ(stdvec.size(), (size_t)std::distance(ivm.begin(), ivm.end()));
        }
        sdsl::remove(temp_dir + "/int_vector_mapper_push_backtest");
    }

    // test variable width
    for (auto const & size : vec_sizes)
    {
        sdsl::int_vector<> vec(size);
        std::vector<uint64_t> stdvec(size);
        sdsl::util::set_to_id(vec);
        sdsl::util::set_to_id(stdvec);
        sdsl::util::bit_compress(vec);
        store_to_file(vec, temp_dir + "/int_vector_mapper_push_backtest");
        {
            sdsl::int_vector_mapper<> ivm(temp_dir + "/int_vector_mapper_push_backtest");
            ASSERT_EQ(size, ivm.size());
            for (size_t i = 0; i < size; i++)
            {
                stdvec.push_back(i);
                ivm.push_back(i);
            }
        }
        {
            sdsl::int_vector_mapper<> ivm(temp_dir + "/int_vector_mapper_push_backtest");
            ASSERT_EQ(stdvec.size(), ivm.size());
            ASSERT_TRUE(std::equal(ivm.begin(), ivm.end(), stdvec.begin()));
            ASSERT_EQ(stdvec.size(), (size_t)std::distance(ivm.begin(), ivm.end()));
        }
        sdsl::remove(temp_dir + "/int_vector_mapper_push_backtest");
    }
}

TEST_F(int_vector_mapper_test, bit_compress)
{
    for (auto const & size : vec_sizes)
    {
        sdsl::int_vector<> vec(size);
        sdsl::util::set_to_id(vec);
        store_to_file(vec, temp_dir + "/int_vector_mapper_bit_compress_test_uncompressed");
        sdsl::util::bit_compress(vec);
        store_to_file(vec, temp_dir + "/int_vector_mapper_bit_compress_test");
        {
            sdsl::int_vector_mapper<> ivmc(temp_dir + "/int_vector_mapper_bit_compress_test");
            sdsl::int_vector_mapper<> ivmu(temp_dir + "/int_vector_mapper_bit_compress_test_uncompressed");
            ASSERT_TRUE(std::equal(ivmc.begin(), ivmc.end(), ivmu.begin()));
            ASSERT_TRUE(std::equal(ivmc.begin(), ivmc.end(), vec.begin()));
        }
        {
            sdsl::int_vector_mapper<> ivmu(temp_dir + "/int_vector_mapper_bit_compress_test_uncompressed");
            ASSERT_TRUE(std::equal(ivmu.begin(), ivmu.end(), vec.begin()));
            sdsl::util::bit_compress(ivmu);
            ASSERT_TRUE(std::equal(ivmu.begin(), ivmu.end(), vec.begin()));
        }
        {
            sdsl::int_vector_mapper<> ivmc(temp_dir + "/int_vector_mapper_bit_compress_test");
            sdsl::int_vector_mapper<> ivmu(temp_dir + "/int_vector_mapper_bit_compress_test_uncompressed");
            ASSERT_EQ(ivmc.size(), ivmu.size());
            ASSERT_EQ(ivmc.width(), ivmu.width());
            ASSERT_TRUE(std::equal(ivmc.begin(), ivmc.end(), ivmu.begin()));
        }
        sdsl::remove(temp_dir + "/int_vector_mapper_bit_compress_test_uncompressed");
        sdsl::remove(temp_dir + "/int_vector_mapper_bit_compress_test");
    }
}

TEST_F(int_vector_mapper_test, bitvector_mapping)
{
    for (auto const & size : vec_sizes)
    {
        sdsl::bit_vector bv(size);
        sdsl::util::set_random_bits(bv, 4711);
        store_to_file(bv, temp_dir + "/bit_vector_mapper_test");
        {
            // load/store test
            sdsl::bit_vector_mapper<std::ios_base::in> const bvm(temp_dir + "/bit_vector_mapper_test");
            ASSERT_EQ(bvm.size(), bv.size());
            ASSERT_EQ(bvm.width(), bv.width());
            ASSERT_TRUE(std::equal(bvm.begin(), bvm.end(), bv.begin()));
            ASSERT_EQ(sdsl::util::cnt_one_bits(bv), sdsl::util::cnt_one_bits(bvm));
        }
        {
            // flip test
            sdsl::bit_vector_mapper<> bvm(temp_dir + "/bit_vector_mapper_test");
            bvm.flip();
            bv.flip();
            ASSERT_TRUE(std::equal(bvm.begin(), bvm.end(), bv.begin()));
            ASSERT_EQ(sdsl::util::cnt_one_bits(bv), sdsl::util::cnt_one_bits(bvm));
        }
        {
            // load/store after flip
            sdsl::bit_vector_mapper<std::ios_base::in> const bvm(temp_dir + "/bit_vector_mapper_test");
            ASSERT_TRUE(std::equal(bvm.begin(), bvm.end(), bv.begin()));
            ASSERT_EQ(sdsl::util::cnt_one_bits(bv), sdsl::util::cnt_one_bits(bvm));
        }
        sdsl::remove(temp_dir + "/bit_vector_mapper_test");
    }
}

TEST_F(int_vector_mapper_test, temp_buffer_test)
{
    for (auto const & size : vec_sizes)
    {
        sdsl::int_vector<> vec(size);
        sdsl::util::set_to_id(vec);
        std::string tmp_file_name;
        {
            auto tmp_buf = sdsl::temp_file_buffer<31>::create();
            tmp_file_name = tmp_buf.file_name();
            ASSERT_EQ(tmp_buf.width(), (uint8_t)31);
            ASSERT_EQ(tmp_buf.size(), (size_t)0);
            ASSERT_TRUE(tmp_buf.empty());
            for (auto const & val : vec)
            {
                tmp_buf.push_back(val);
            }
            ASSERT_EQ(tmp_buf.size(), vec.size());
            ASSERT_TRUE(std::equal(tmp_buf.begin(), tmp_buf.end(), vec.begin()));
        }
        // check that the file is gone
        sdsl::isfstream cfs(tmp_file_name);
        ASSERT_FALSE(cfs.is_open());
    }
}

TEST_F(int_vector_mapper_test, mapping_failure_releases_descriptor)
{
    if (sdsl::is_ram_file(temp_dir))
        GTEST_SKIP() << "Read-only OS mapping requires a disk file";
    auto const file = temp_dir + "/mapper_empty_file";
    {
        std::ofstream out(file, std::ios::binary);
    }
    int const before = ::open(file.c_str(), O_RDONLY);
    ASSERT_GE(before, 0);
    ::close(before);
    using mapper_type = sdsl::int_vector_mapper<8, std::ios_base::in>;
    EXPECT_THROW(mapper_type(file, true), std::runtime_error);
    int const after = ::open(file.c_str(), O_RDONLY);
    ASSERT_GE(after, 0);
    ::close(after);
    EXPECT_EQ(before, after);
    sdsl::remove(file);
}

TEST_F(int_vector_mapper_test, missing_plain_file_is_rejected)
{
    std::string const file = temp_dir + "/int_vector_mapper_missing_plain";
    sdsl::remove(file);
    EXPECT_THROW((sdsl::int_vector_mapper<8, std::ios_base::in>(file, true)), std::runtime_error);
    EXPECT_FALSE(sdsl::is_ram_file(file) ? sdsl::ram_fs::exists(file) : std::ifstream(file).good());
}

TEST_F(int_vector_mapper_test, header_larger_than_file_is_rejected)
{
    std::string const file = temp_dir + "/int_vector_mapper_truncated";
    {
        // The header claims 1 MiB of payload, but the file holds 16 bytes.
        sdsl::osfstream out(file, std::ios::binary | std::ios::trunc | std::ios::out);
        sdsl::int_vector<8>::write_header(uint64_t{1} << 23, 8, out);
        uint64_t const payload[2] = {~uint64_t{0}, ~uint64_t{0}};
        out.write(reinterpret_cast<char const *>(payload), sizeof(payload));
    }
    EXPECT_THROW((sdsl::int_vector_mapper<8, std::ios_base::in>(file)), std::runtime_error);
    {
        // The header claims 9 bytes, which int_vector stores in two whole words.
        sdsl::osfstream out(file, std::ios::binary | std::ios::trunc | std::ios::out);
        sdsl::int_vector<8>::write_header(72, 8, out);
        uint8_t const payload[9] = {};
        out.write(reinterpret_cast<char const *>(payload), sizeof(payload));
    }
    EXPECT_THROW((sdsl::int_vector_mapper<8, std::ios_base::in>(file)), std::runtime_error);
    sdsl::remove(file);
}

TEST_F(int_vector_mapper_test, read_only_mappings_release_os_descriptors)
{
    if (sdsl::is_ram_file(temp_dir))
        GTEST_SKIP() << "Descriptor reuse requires a disk file";
    auto const file = temp_dir + "/mapper_read_only_descriptor";
    ASSERT_TRUE(sdsl::store_to_file(sdsl::int_vector<64>(8, 7), file));
    int const before = ::open(file.c_str(), O_RDONLY);
    ASSERT_GE(before, 0);
    ::close(before);
    using mapper_type = sdsl::int_vector_mapper<64, std::ios_base::in>;
    std::vector<mapper_type> mappers;
    for (size_t i = 0; i < 64; ++i)
        mappers.emplace_back(file);
    int const reused = ::open(file.c_str(), O_RDONLY);
    ASSERT_GE(reused, 0);
    EXPECT_EQ(before, reused);
    for (auto const & mapper : mappers)
        EXPECT_EQ(7u, mapper[7]);
    mappers.clear();
    // Releasing a mapping must not close a descriptor reused by another file.
    EXPECT_EQ(0, ::close(reused));
    sdsl::remove(file);
}

TEST_F(int_vector_mapper_test, writable_mappings_keep_os_descriptors)
{
    if (sdsl::is_ram_file(temp_dir))
        GTEST_SKIP() << "Descriptor reuse requires a disk file";
    auto const file = temp_dir + "/mapper_writable_descriptor";
    ASSERT_TRUE(sdsl::store_to_file(sdsl::int_vector<64>(8, 7), file));
    int const before = ::open(file.c_str(), O_RDONLY);
    ASSERT_GE(before, 0);
    ::close(before);
    {
        sdsl::int_vector_mapper<64> mapper(file);
        int const next = ::open(file.c_str(), O_RDONLY);
        ASSERT_GE(next, 0);
        // The mapper still holds the lowest free descriptor.
        EXPECT_NE(before, next);
        ::close(next);
        mapper[0] = 9;
    }
    sdsl::int_vector<64> stored;
    ASSERT_TRUE(sdsl::load_from_file(stored, file));
    EXPECT_EQ(9u, stored[0]);
    sdsl::remove(file);
}

TEST_F(int_vector_mapper_test, read_only_delete_on_close_survives_moves)
{
    using mapper_type = sdsl::int_vector_mapper<64, std::ios_base::in>;
    auto const first = temp_dir + "/mapper_delete_first";
    auto const second = temp_dir + "/mapper_delete_second";
    sdsl::int_vector<64> values(8, 7);
    ASSERT_TRUE(sdsl::store_to_file(values, first));
    ASSERT_TRUE(sdsl::store_to_file(values, second));
    {
        mapper_type source(first, false, true);
        {
            mapper_type destination(second, false, true);
            destination = std::move(source);
            EXPECT_FALSE(sdsl::isfstream(second).is_open());
            mapper_type moved(std::move(destination));
            EXPECT_EQ(7u, std::as_const(moved)[0]);
            EXPECT_TRUE(sdsl::isfstream(first).is_open());
        }
        EXPECT_FALSE(sdsl::isfstream(first).is_open());
        ASSERT_TRUE(sdsl::store_to_file(values, first));
    }
    // The moved-from object must not delete a newly created file at that path.
    EXPECT_TRUE(sdsl::isfstream(first).is_open());
    sdsl::remove(first);
}

// Indexed reads load up to 7 bytes after an element, so the end of a mapping must stay readable. 8191 words
// and the 8-byte header of a fixed-width vector make a 65536-byte file, which ends on a page boundary for 4,
// 16 and 64 KiB pages. A RAM file ends at its last word.
template <uint8_t t_width>
void check_tail_reads(std::string const & filename, uint8_t width)
{
    sdsl::int_vector<t_width> expected(8191 * 64 / width, 0, width);
    std::mt19937_64 random(width);
    for (auto && value : expected)
        value = random();
    ASSERT_TRUE(sdsl::store_to_file(expected, filename));
    {
        sdsl::int_vector_mapper<t_width, std::ios_base::in> const mapped(filename);
        ASSERT_EQ(mapped.size(), expected.size());
        auto const first = mapped.begin();
        for (size_t i = expected.size() - 64; i < expected.size(); ++i)
        {
            ASSERT_EQ(mapped[i], expected[i]);
            ASSERT_EQ(first[i], expected[i]);
        }
    }
    {
        // Growing a writable mapping maps the file again.
        sdsl::int_vector_mapper<t_width> mapped(filename);
        auto const & view = mapped;
        for (size_t i = expected.size() - 64; i < expected.size(); ++i)
            ASSERT_EQ(view[i], expected[i]);
        mapped.push_back(1);
        ASSERT_EQ(view[expected.size()], 1u);
        ASSERT_EQ(view.begin()[expected.size() - 1], expected[expected.size() - 1]);
    }
    sdsl::remove(filename);
}

TEST_F(int_vector_mapper_test, indexed_reads_at_the_end_of_the_mapping)
{
    check_tail_reads<5>(temp_dir + "/mapper_tail_fixed", 5);
    check_tail_reads<13>(temp_dir + "/mapper_tail_fixed", 13);
    check_tail_reads<57>(temp_dir + "/mapper_tail_fixed", 57);
    check_tail_reads<0>(temp_dir + "/mapper_tail_dynamic", 13);
    check_tail_reads<0>(temp_dir + "/mapper_tail_dynamic", 57);
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
    bool res = RUN_ALL_TESTS();
    temp_dir = "@";
    return RUN_ALL_TESTS() or res;
}
