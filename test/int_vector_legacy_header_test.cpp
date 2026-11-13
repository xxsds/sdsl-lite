#include <cstddef>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

#include <sdsl/int_vector.hpp>
#include <sdsl/int_vector_buffer.hpp>
#include <sdsl/int_vector_mapper.hpp>
#include <sdsl/packed_vector_view.hpp>
#include <sdsl/ram_fs.hpp>
#include <sdsl/shared_mapper.hpp>

#include "gtest/gtest.h"

namespace
{

std::string temp_file;

// The sdsl v2 layout: the size in bits as a whole word, then for int_vector<0>
// a byte with the width, then the payload words.
template <uint8_t t_width>
std::string v2_bytes(sdsl::int_vector<t_width> const & v)
{
    std::string out;
    uint64_t const bits = v.bit_size();
    out.append(reinterpret_cast<char const *>(&bits), sizeof(bits));
    if (t_width == 0)
        out.push_back(static_cast<char>(v.width()));
    out.append(reinterpret_cast<char const *>(v.data()), ((bits + 63) / 64) * 8);
    return out;
}

template <uint8_t t_width>
void write_v2(sdsl::int_vector<t_width> const & v)
{
    std::ofstream(temp_file, std::ios::binary) << v2_bytes(v);
}

sdsl::int_vector<> values(size_t n, uint8_t width)
{
    sdsl::int_vector<> v(n, 0, width);
    for (size_t i = 0; i < n; ++i)
        v[i] = (i * 7919) & sdsl::bits::lo_set[width];
    return v;
}

// The files are written and read with OS file streams, so they must be on disk.
class int_vector_legacy_header_test : public ::testing::Test
{
protected:
    void SetUp() override
    {
        if (sdsl::is_ram_file(temp_file))
            GTEST_SKIP() << "The test writes files with std::ofstream";
    }
};

TEST_F(int_vector_legacy_header_test, stream_load)
{
    for (size_t n : {0, 1, 1000})
    {
        auto const v = values(n, 13);
        std::istringstream in(v2_bytes(v));
        sdsl::int_vector<> loaded;
        loaded.load(in);
        EXPECT_EQ(v, loaded);
        EXPECT_EQ(13u, loaded.width());

        sdsl::bit_vector bits(n + 3, 0);
        for (size_t i = 0; i < bits.size(); i += 3)
            bits[i] = 1;
        std::istringstream bits_in(v2_bytes(bits));
        sdsl::bit_vector loaded_bits;
        loaded_bits.load(bits_in);
        EXPECT_EQ(bits, loaded_bits);
    }
}

TEST_F(int_vector_legacy_header_test, current_header_is_unchanged)
{
    auto const v = values(1000, 13);
    std::ostringstream out;
    v.serialize(out);
    uint64_t word = 0;
    out.str().copy(reinterpret_cast<char *>(&word), sizeof(word));
    EXPECT_EQ(13u, word >> 56);
    std::istringstream in(out.str());
    sdsl::int_vector<> loaded;
    loaded.load(in);
    EXPECT_EQ(v, loaded);
}

TEST_F(int_vector_legacy_header_test, mapped_view_starts_after_the_longer_header)
{
    auto const v = values(1000, 13);
    write_v2(v);
    sdsl::packed_vector_view<0> view;
    {
        sdsl::shared_mapper_ifstream in(temp_file);
        view.load(in);
        EXPECT_EQ(reinterpret_cast<std::byte const *>(in.mapper()->data()) + 9, view.bytes().data());
    }
    EXPECT_EQ(v, view.to_owned());
}

TEST_F(int_vector_legacy_header_test, buffer_reads_and_refuses_to_write)
{
    auto const v = values(1000, 13);
    write_v2(v);
    {
        sdsl::int_vector_buffer<> buffer(temp_file, std::ios::in);
        ASSERT_EQ(v.size(), buffer.size());
        EXPECT_EQ(13u, buffer.width());
        for (size_t i = 0; i < v.size(); ++i)
            ASSERT_EQ(v[i], buffer[i]) << "i=" << i;
    }
    EXPECT_THROW((sdsl::int_vector_buffer<>(temp_file, std::ios::in | std::ios::out)), std::invalid_argument);
    std::ifstream in(temp_file, std::ios::binary);
    std::string after((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    EXPECT_EQ(v2_bytes(v), after);

    sdsl::int_vector<8> bytes(100, 0);
    for (size_t i = 0; i < bytes.size(); ++i)
        bytes[i] = i;
    write_v2(bytes);
    sdsl::int_vector_buffer<8> fixed(temp_file, std::ios::in);
    ASSERT_EQ(bytes.size(), fixed.size());
    for (size_t i = 0; i < bytes.size(); ++i)
        ASSERT_EQ(bytes[i], fixed[i]);
}

TEST_F(int_vector_legacy_header_test, mapper_maps_fixed_width_and_refuses_unaligned_payload)
{
    sdsl::bit_vector bits(1000, 0);
    for (size_t i = 0; i < bits.size(); i += 7)
        bits[i] = 1;
    write_v2(bits);
    {
        auto const mapped = sdsl::read_only_mapper<1>(temp_file);
        ASSERT_EQ(bits.size(), mapped.size());
        EXPECT_TRUE(std::equal(bits.begin(), bits.end(), mapped.begin()));
    }
    write_v2(values(1000, 13));
    EXPECT_THROW((void)sdsl::read_only_mapper<>(temp_file), std::runtime_error);
}

} // namespace

int main(int argc, char ** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " tmp_dir" << std::endl;
        return 1;
    }
    temp_file = std::string(argv[1]) + "/int_vector_legacy_header_test_" + std::to_string(sdsl::util::pid());
    int const result = RUN_ALL_TESTS();
    std::remove(temp_file.c_str());
    return result;
}
