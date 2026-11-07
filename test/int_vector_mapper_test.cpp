#include <algorithm>
#include <fcntl.h>
#include <random>
#include <sstream>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>

#include <sdsl/int_vector_mapper.hpp>
#include <sdsl/packed_vector_view.hpp>
#include <sdsl/rank_support_v5.hpp>
#include <sdsl/select_support_mcl.hpp>
#include <sdsl/select_support_scan.hpp>
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
    if (temp_dir == "@")
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

// The view receives payload boundaries, not a serialized vector header.
TEST_F(int_vector_mapper_test, packed_view_rank_select)
{
    if (temp_dir == "@")
        GTEST_SKIP() << "Read-only OS mapping requires a disk file";
    using view_type = sdsl::packed_vector_view<1>;
    auto const file = temp_dir + "/packed_view_payload";
    for (size_type size : {0, 1, 63, 64, 65, 2047, 2048, 2049, 100003})
    {
        for (unsigned offset = 0; offset < 8; ++offset)
        {
            sdsl::bit_vector expected(size, 0);
            for (size_type i = 0; i < size; ++i)
                expected[i] = (i * 17 % 101) < 43;
            {
                std::ofstream out(file, std::ios::binary);
                out << std::string(offset, 'x');
                if (size)
                    out.write(reinterpret_cast<char const *>(expected.data()), ((size + 63) / 64) * 8);
                out.put('!'); // permits an empty payload in an otherwise nonempty mapping
            }
            auto owner = std::make_shared<view_type::byte_mapper const>(file, true);
            auto view = view_type::from_mapping(owner, offset, size);
            auto alias = view;
            owner.reset();
            sdsl::rank_support_v5<1, 1, view_type> rank(&view);
            sdsl::rank_support_v5<0, 1, view_type> rank_zero(&view);
            sdsl::select_support_mcl<1, 1, view_type> select(&view);
            sdsl::select_support_mcl<0, 1, view_type> select_zero(&view);
            sdsl::select_support_scan<0, 1, view_type> scan_zero(&view);
            size_type ones = 0, zeros = 0;
            for (size_type i = 0; i < size; ++i)
            {
                ASSERT_EQ(ones, rank(i));
                ASSERT_EQ(zeros, rank_zero(i));
                ASSERT_EQ(expected[i], view[i]);
                if (expected[i])
                    ASSERT_EQ(i, select(++ones));
                else
                {
                    ASSERT_EQ(i, select_zero(++zeros));
                    if (zeros <= 4)
                        ASSERT_EQ(i, scan_zero(zeros));
                }
            }
            ASSERT_EQ(ones, rank(size));
            ASSERT_EQ(zeros, rank_zero(size));
            std::stringstream rs, ss;
            rank.serialize(rs);
            select.serialize(ss);
            sdsl::rank_support_v5<1> native_rank;
            sdsl::select_support_mcl<1> native_select;
            native_rank.load(static_cast<std::istream &>(rs), &expected);
            native_select.load(static_cast<std::istream &>(ss), &expected);
            ASSERT_EQ(ones, native_rank(size));
            if (ones)
                ASSERT_EQ(select(ones), native_select(ones));
            auto owned = std::move(view).into_owned();
            ASSERT_EQ(expected, owned);
            owned.resize(size + 65);
            owned[size] = 1;
            ASSERT_EQ(expected, alias.to_owned());
        }
    }
    sdsl::remove(file);
}

TEST_F(int_vector_mapper_test, packed_view_ownership_and_bounds)
{
    sdsl::int_vector<> partial(43, 7, 3);
    partial.width(5);
    sdsl::packed_vector_view<> partial_view(std::move(partial));
    ASSERT_EQ(129u, partial_view.to_owned().bit_size());
    ASSERT_EQ(partial_view.get_int(125, 4), partial_view.to_owned().get_int(125, 4));
    for (unsigned width : {1, 3, 5, 13, 32, 64})
    {
        sdsl::int_vector<> values(129, 7, width);
        sdsl::packed_vector_view<> view(std::move(values));
        auto const address = view.bytes().data();
        auto owned = std::move(view).into_owned();
        ASSERT_EQ(address, reinterpret_cast<std::byte const *>(owned.data()));
        ASSERT_TRUE(view.empty());
        view = sdsl::packed_vector_view<>(std::move(owned));
        auto alias = view;
        auto copy = std::move(view).into_owned();
        copy[0] = 0;
        ASSERT_NE(copy[0], alias[0]);
        ASSERT_EQ(129u, alias.size());
        ASSERT_EQ(width, alias.width());
    }
    auto bytes = std::make_shared<std::vector<std::byte>>(8);
    using view_type = sdsl::packed_vector_view<1>;
    EXPECT_THROW(view_type::from_bytes(bytes, *bytes, 65), std::invalid_argument);
    EXPECT_THROW(view_type::from_bytes(bytes, *bytes, 64, 2), std::invalid_argument);
    EXPECT_THROW(view_type::from_bytes({}, *bytes, 64), std::invalid_argument);
    EXPECT_THROW(view_type::from_bytes(bytes, *bytes, UINT64_MAX), std::invalid_argument);
}

TEST_F(int_vector_mapper_test, packed_view_reuses_const_iterator)
{
    using view_type = sdsl::packed_vector_view<>;
    static_assert(std::same_as<view_type::const_iterator, sdsl::random_access_const_iterator<view_type>>);
    static_assert(std::random_access_iterator<view_type::const_iterator>);
    view_type empty;
    EXPECT_EQ(empty.end() - empty.begin(), 0);
    EXPECT_EQ(view_type::const_iterator{} - view_type::const_iterator{}, 0);
    for (uint8_t width = 1; width <= 64; ++width)
    {
        for (uint64_t size : {0, 1, 63, 64, 65, 129, 257})
        {
            sdsl::int_vector<> values(size, 0, width);
            for (uint64_t i = 0; i < size; ++i)
                values[i] = (i * 0x9e3779b97f4a7c15ULL) ^ (i >> 2);
            auto const & native = values;
            for (size_t offset = 0; offset < 8; ++offset)
            {
                auto bytes = std::make_shared<std::vector<std::byte>>(offset + (((values.bit_size() + 63) / 64) * 8));
                if (values.bit_size())
                    std::memcpy(bytes->data() + offset, values.data(), ((values.bit_size() + 63) / 64) * 8);
                auto view = view_type::from_bytes(bytes, std::span(*bytes).subspan(offset), values.bit_size(), width);
                EXPECT_EQ(view.end() - view.begin(), ptrdiff_t(size));
                EXPECT_EQ(view.begin() - view.end(), -ptrdiff_t(size));
                auto first = view.begin(), last = view.end();
                for (uint64_t i = 0; i < size; ++i)
                {
                    EXPECT_EQ(*first++, native[i]);
                    EXPECT_EQ(*--last, native[size - i - 1]);
                    EXPECT_EQ(view.begin()[i], native[i]);
                    EXPECT_EQ(*(ptrdiff_t(i) + view.begin()), native[i]);
                    auto cursor = view.end();
                    cursor += ptrdiff_t(i) - ptrdiff_t(size);
                    EXPECT_EQ(*cursor, native[i]);
                    cursor -= ptrdiff_t(i);
                    EXPECT_EQ(cursor, view.begin());
                }
                EXPECT_EQ(first, view.end());
                EXPECT_EQ(last, view.begin());
                // Decoder equivalence at every bit offset, including cross-word reads.
                for (uint64_t bit = 0; bit < std::min<uint64_t>(129, values.bit_size()); ++bit)
                    for (uint8_t length = 1; length <= std::min<uint64_t>(64, values.bit_size() - bit); ++length)
                        EXPECT_EQ(view.get_int(bit, length), native.get_int(bit, length));
            }
        }
    }
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
