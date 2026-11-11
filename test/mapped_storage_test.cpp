// Types with view storage read their payloads from a shared_mapper_ifstream
// without copying them, and behave like the owned types.
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <random>
#include <span>
#include <sstream>
#include <string>
#include <system_error>
#include <unistd.h>
#include <vector>

#include <sdsl/packed_vector_view.hpp>
#include <sdsl/rank_support_v.hpp>
#include <sdsl/rank_support_v5.hpp>
#include <sdsl/select_support_mcl.hpp>
#include <sdsl/select_support_scan.hpp>
#include <sdsl/shared_mapper.hpp>
#include <sdsl/util.hpp>

#include <sys/mman.h>

#include <gtest/gtest.h>

namespace
{

using sdsl::packed_vector_view;

std::string temp_dir;

class mapped_storage_test : public ::testing::Test
{
protected:
    std::string file = temp_dir + "/mapped_storage_" + std::to_string(sdsl::util::pid()) + ".sdsl";
    void SetUp() override
    {
        if (sdsl::is_ram_file(temp_dir))
            GTEST_SKIP() << "Read-only OS mapping requires a disk file";
    }
    void TearDown() override
    {
        std::remove(file.c_str());
    }

    // Write `prefix` padding bytes and then the serialization of `value`.
    template <class T>
    std::string write(T const & value, size_t prefix)
    {
        std::ostringstream out;
        out << std::string(prefix, 'x');
        value.serialize(out);
        std::ofstream(file, std::ios::binary) << out.str();
        return out.str().substr(prefix);
    }
};

template <class T>
std::string serialized(T const & value)
{
    std::ostringstream out;
    value.serialize(out);
    return out.str();
}

sdsl::bit_vector random_bits(uint64_t size, double density, unsigned seed)
{
    std::mt19937_64 rng(seed);
    std::bernoulli_distribution bit(density);
    sdsl::bit_vector bits(size, 0);
    for (uint64_t i = 0; i < size; ++i)
        bits[i] = bit(rng);
    return bits;
}

static_assert(std::is_same_v<packed_vector_view<1>::rank_1_type,
                             sdsl::rank_support_v<1, 1, packed_vector_view<1>, packed_vector_view>>);
static_assert(std::is_same_v<sdsl::bit_vector::rank_1_type, sdsl::rank_support_v<1, 1, sdsl::bit_vector>>);
static_assert(std::is_same_v<sdsl::select_support_mcl<>, sdsl::select_support_mcl<1, 1, sdsl::bit_vector>>);

TEST_F(mapped_storage_test, view_load_maps_and_copies)
{
    sdsl::int_vector<> values(1000, 0, 13);
    for (size_t i = 0; i < values.size(); ++i)
        values[i] = (i * 7919) & 0x1fff;
    for (size_t prefix = 0; prefix < 9; ++prefix)
    {
        auto const bytes = write(values, prefix);
        packed_vector_view<0> mapped, copied;
        {
            sdsl::shared_mapper_ifstream in(file, std::ios::binary);
            in.seekg(prefix);
            mapped.load(in);
            EXPECT_EQ(prefix + bytes.size(), static_cast<size_t>(in.tellg()));
            EXPECT_EQ(reinterpret_cast<std::byte const *>(in.mapper()->data()) + prefix + 8, mapped.bytes().data());
        }
        {
            std::ifstream in(file, std::ios::binary);
            in.seekg(prefix);
            copied.load(in);
            EXPECT_EQ(prefix + bytes.size(), static_cast<size_t>(in.tellg()));
        }
        std::remove(file.c_str()); // The mapping keeps the payload alive.
        ASSERT_EQ(values.size(), mapped.size());
        for (size_t i = 0; i < values.size(); ++i)
        {
            ASSERT_EQ(values[i], mapped[i]);
            ASSERT_EQ(values[i], copied[i]);
        }
        EXPECT_EQ(bytes, serialized(mapped));
        EXPECT_EQ(bytes, serialized(copied));
        EXPECT_EQ(serialized(values), bytes);
    }
}

TEST_F(mapped_storage_test, fixed_width_view_serializes_like_int_vector)
{
    auto const bits = random_bits(1000, 0.5, 3);
    auto const bytes = write(bits, 5);
    packed_vector_view<1> mapped;
    {
        sdsl::shared_mapper_ifstream in(file, std::ios::binary);
        in.seekg(5);
        mapped.load(in);
    }
    EXPECT_EQ(1, mapped.width());
    EXPECT_EQ(bytes, serialized(mapped));
    EXPECT_EQ(bytes, serialized(packed_vector_view<1>(bits)));
    EXPECT_EQ(serialized(sdsl::bit_vector()), serialized(packed_vector_view<1>()));
    EXPECT_EQ(serialized(sdsl::int_vector<>()), serialized(packed_vector_view<0>()));
}

TEST_F(mapped_storage_test, view_load_rejects_truncated_input)
{
    sdsl::bit_vector bits(1000, 1);
    auto const bytes = write(bits, 0);
    for (size_t keep : {bytes.size() - 8, size_t{4}, size_t{0}})
    {
        std::ofstream(file, std::ios::binary) << bytes.substr(0, keep);
        packed_vector_view<1> view;
        {
            sdsl::shared_mapper_ifstream in(file, std::ios::binary);
            EXPECT_EQ(keep == 0, in.mapper() == nullptr);
            if (keep == bytes.size() - 8)
                EXPECT_THROW(view.load(in), std::invalid_argument);
            else
                EXPECT_THROW(view.load(in), std::ios_base::failure);
        }
        {
            std::ifstream in(file, std::ios::binary);
            EXPECT_THROW(view.load(in), std::ios_base::failure);
        }
        EXPECT_EQ(0u, view.size());
    }
}

// Expected select answers for the pattern of `Select`.
template <class Select>
std::vector<uint64_t> pattern_positions(sdsl::bit_vector const & bits)
{
    std::vector<uint64_t> positions;
    for (uint64_t j = Select::bit_pat_len - 1; j < bits.size(); ++j)
    {
        bool found = true;
        for (uint8_t k = 0; k < Select::bit_pat_len; ++k)
            found &= bits[j - k] == ((Select::bit_pat >> k) & 1);
        if (found)
            positions.push_back(j);
    }
    return positions;
}

template <class Select, class Bits>
void check_select(sdsl::bit_vector const & bits, Bits const & loaded, Select const & select)
{
    ASSERT_EQ(bits.size(), loaded.size());
    for (uint64_t i = 0; i < bits.size(); ++i)
        ASSERT_EQ(bits[i], loaded[i]);
    auto const positions = pattern_positions<Select>(bits);
    for (uint64_t i = 0; i < positions.size(); ++i)
        ASSERT_EQ(positions[i], select.select(i + 1));
}

template <class Rank, class Bits>
void check_rank(sdsl::bit_vector const & bits, Bits const & loaded, Rank const & rank)
{
    uint64_t ones = 0;
    for (uint64_t i = 0; i <= bits.size(); ++i)
    {
        ASSERT_EQ(ones, rank.rank(i));
        if (i < bits.size())
        {
            ASSERT_EQ(bits[i], loaded[i]);
            ones += bits[i] == Rank::bit_pat;
        }
    }
}

// Build a support with view storage over a view bit vector, then load both
// from a mapped stream and from a plain one at several byte offsets. The
// serialization must equal that of `Reference`, the owned type.
template <class Support, class Reference, class Check>
void check_support_storage(std::string const & file, sdsl::bit_vector const & bits, Check check)
{
    packed_vector_view<1> owned_bits(bits);
    Support owned(&owned_bits);
    Reference reference(&bits);
    auto const ref_bytes = serialized(reference);
    ASSERT_EQ(ref_bytes, serialized(owned));
    for (size_t prefix : {0, 3, 8})
    {
        {
            std::ostringstream out;
            out << std::string(prefix, 'x');
            owned_bits.serialize(out);
            owned.serialize(out);
            std::ofstream(file, std::ios::binary) << out.str();
        }
        packed_vector_view<1> mapped_bits, copied_bits;
        Support mapped, copied;
        {
            sdsl::shared_mapper_ifstream in(file, std::ios::binary);
            in.seekg(prefix);
            mapped_bits.load(in);
            mapped.load(in, &mapped_bits);
            ASSERT_TRUE(in);
            // The stream, the bit vector and the rank samples share the mapping.
            ASSERT_EQ(3, in.mapper().use_count());
            ASSERT_EQ(prefix + serialized(owned_bits).size() + ref_bytes.size(), static_cast<size_t>(in.tellg()));
        }
        {
            std::ifstream in(file, std::ios::binary);
            in.seekg(prefix);
            copied_bits.load(in);
            copied.load(in, &copied_bits);
        }
        std::remove(file.c_str()); // The mapping keeps the payload alive.
        check(bits, mapped_bits, mapped);
        check(bits, copied_bits, copied);
        EXPECT_TRUE(mapped == copied);
        EXPECT_EQ(ref_bytes, serialized(mapped));
        EXPECT_EQ(ref_bytes, serialized(copied));
        // Copies and moves keep the mapping alive.
        Support copy(mapped);
        Support assigned;
        assigned = copy;
        Support moved(std::move(mapped));
        mapped = Support();
        check(bits, mapped_bits, copy);
        check(bits, mapped_bits, assigned);
        check(bits, mapped_bits, moved);
        EXPECT_EQ(ref_bytes, serialized(moved));
    }
}

template <class Check>
void for_each_bits(Check check)
{
    unsigned seed = 1;
    // Below 100000 bits MCL uses its slow construction, above it the fast one.
    // 4096 ends at a superblock of rank_support_v (512 bits) and rank_support_v5 (2048 bits).
    for (uint64_t size : {0ull, 1ull, 64ull, 2049ull, 4096ull, 5000ull, 300000ull})
        for (double density : {0.001, 0.5, 0.999})
            check(random_bits(size, density, seed++));
}

TEST_F(mapped_storage_test, rank_v5_view_storage)
{
    for_each_bits(
        [&](sdsl::bit_vector const & bits)
        {
            using V = packed_vector_view<1>;
            using R1 = sdsl::rank_support_v5<1, 1, V, packed_vector_view>;
            using R0 = sdsl::rank_support_v5<0, 1, V, packed_vector_view>;
            check_support_storage<R1, sdsl::rank_support_v5<1, 1>>(file, bits, check_rank<R1, V>);
            check_support_storage<R0, sdsl::rank_support_v5<0, 1>>(file, bits, check_rank<R0, V>);
        });
}

TEST_F(mapped_storage_test, rank_v_view_storage)
{
    for_each_bits(
        [&](sdsl::bit_vector const & bits)
        {
            using V = packed_vector_view<1>;
            using R1 = V::rank_1_type;
            using R0 = V::rank_0_type;
            check_support_storage<R1, sdsl::rank_support_v<1, 1>>(file, bits, check_rank<R1, V>);
            check_support_storage<R0, sdsl::rank_support_v<0, 1>>(file, bits, check_rank<R0, V>);
        });
}

// Select supports over a mapped view bit vector answer as over the owned one.
TEST_F(mapped_storage_test, select_over_mapped_view)
{
    using V = packed_vector_view<1>;
    auto check = [&]<uint8_t t_b, uint8_t t_pat_len>(sdsl::bit_vector const & bits)
    {
        using Select = sdsl::select_support_mcl<t_b, t_pat_len, V>;
        sdsl::select_support_mcl<t_b, t_pat_len> reference(&bits);
        {
            std::ostringstream out;
            out << "xyz";
            bits.serialize(out);
            reference.serialize(out);
            std::ofstream(file, std::ios::binary) << out.str();
        }
        V mapped;
        Select loaded; // Its samples are copied from the mapped stream.
        sdsl::shared_mapper_ifstream in(file, std::ios::binary);
        in.seekg(3);
        mapped.load(in);
        loaded.load(in, &mapped);
        ASSERT_TRUE(in);
        Select select(&mapped);
        sdsl::select_support_scan<t_b, t_pat_len, V> scan(&mapped);
        EXPECT_EQ(serialized(reference), serialized(select));
        EXPECT_EQ(serialized(reference), serialized(loaded));
        check_select(bits, mapped, select);
        check_select(bits, mapped, loaded);
        auto const positions = pattern_positions<decltype(select)>(bits);
        for (uint64_t i = 0; i < std::min<uint64_t>(positions.size(), 100); ++i)
            ASSERT_EQ(positions[i], scan.select(i + 1));
    };
    for_each_bits(
        [&](sdsl::bit_vector const & bits)
        {
            check.template operator()<1, 1>(bits);
            check.template operator()<0, 1>(bits);
        });
    auto const bits = random_bits(20000, 0.5, 9);
    check.template operator()<10, 2>(bits);
}

// Copy `size` bytes so that they end right before an inaccessible page. Reading
// past them then faults in any build, not only under AddressSanitizer.
std::span<std::byte const> copy_before_guard_page(std::shared_ptr<void const> & owner, void const * data, size_t size)
{
    size_t const page = static_cast<size_t>(sysconf(_SC_PAGESIZE));
    size_t const length = (size + page - 1) / page * page + page;
    void * const mapping = mmap(nullptr, length, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mapping == MAP_FAILED)
        throw std::system_error(errno, std::generic_category(), "mmap");
    owner = std::shared_ptr<void const>(static_cast<void const *>(mapping),
                                        [length](void const * p)
                                        {
                                            munmap(const_cast<void *>(p), length);
                                        });
    auto * const guard = static_cast<std::byte *>(mapping) + length - page;
    if (mprotect(guard, page, PROT_NONE) != 0)
        throw std::system_error(errno, std::generic_category(), "mprotect");
    std::memcpy(guard - size, data, size);
    return {guard - size, size};
}

// Views have no padding word after a word-aligned end.
TEST(packed_vector_view_test, supports_read_no_word_past_the_payload)
{
    using V = packed_vector_view<1>;
    for (uint64_t size : {64, 4096, 300032}) // MCL builds fast above 100000 bits
    {
        auto const bits = random_bits(size, 0.5, 7);
        std::shared_ptr<void const> owner;
        auto const payload = copy_before_guard_page(owner, bits.data(), size / 8);
        auto const view = V::from_bytes(owner, payload, size);
        auto const ones = sdsl::util::cnt_one_bits(bits);
        EXPECT_EQ(ones, (sdsl::rank_support_v<1, 1, V>(&view).rank(size)));
        EXPECT_EQ(ones, (sdsl::rank_support_v5<1, 1, V>(&view).rank(size)));
        EXPECT_EQ(size - ones, (sdsl::rank_support_v5<0, 1, V>(&view).rank(size)));
        EXPECT_EQ(sdsl::select_support_mcl<1>(&bits).select(ones),
                  (sdsl::select_support_mcl<1, 1, V>(&view).select(ones)));
    }
}

TEST(packed_vector_view_test, ownership_and_bounds)
{
    // 129 bits hold 25 elements of width 5 and four bits more.
    auto partial = []
    {
        sdsl::int_vector<> v(43, 7, 3);
        v.width(5);
        return v;
    };
    packed_vector_view<> view(partial());
    EXPECT_EQ(25u, view.size());
    EXPECT_EQ(partial(), view.to_owned());
    EXPECT_EQ(partial().get_int(125, 4), view.get_int(125, 4));
    auto owned = view.to_owned();
    owned[0] = 0;
    EXPECT_NE(owned[0], view[0]);
    auto bytes = std::make_shared<std::vector<std::byte>>(8);
    using V = packed_vector_view<1>;
    EXPECT_THROW(V::from_bytes(bytes, *bytes, 65), std::invalid_argument);
    EXPECT_THROW(V::from_bytes(bytes, *bytes, 64, 2), std::invalid_argument);
    EXPECT_THROW(V::from_bytes({}, *bytes, 64), std::invalid_argument);
    EXPECT_THROW(V::from_bytes(bytes, *bytes, UINT64_MAX), std::invalid_argument);
}

TEST(packed_vector_view_test, comparison_ignores_padding_and_checks_width)
{
    using V = packed_vector_view<>;
    sdsl::int_vector<> a(65, 1, 1), b(a);
    b.data()[1] |= 2; // padding bit
    auto bytes = std::make_shared<std::vector<std::byte>>(3 + 16);
    std::memcpy(bytes->data() + 3, b.data(), 16);
    auto const mapped = V::from_bytes(bytes, std::span(*bytes).subspan(3), 65, 1);
    EXPECT_EQ(V(a), mapped);
    EXPECT_FALSE(V(a) == V::from_bytes(bytes, std::span(*bytes).subspan(3), 64, 1));
    auto c = a;
    c.width(5);
    EXPECT_FALSE(V(c) == V(a));
    a[64] = 0;
    EXPECT_FALSE(V(a) == mapped);
}

TEST(packed_vector_view_test, const_iterator_and_get_int)
{
    using V = packed_vector_view<>;
    static_assert(std::same_as<V::const_iterator, sdsl::random_access_const_iterator<V>>);
    EXPECT_TRUE(V().empty());
    for (uint8_t width : {1, 13, 64})
    {
        for (uint64_t size : {0, 1, 63, 64, 65, 129})
        {
            sdsl::int_vector<> values(size, 0, width);
            for (uint64_t i = 0; i < size; ++i)
                values[i] = (i * 0x9e3779b97f4a7c15ULL) ^ (i >> 2);
            auto const payload = ((values.bit_size() + 63) / 64) * 8;
            for (size_t offset : {0, 3})
            {
                auto bytes = std::make_shared<std::vector<std::byte>>(offset + payload);
                if (payload)
                    std::memcpy(bytes->data() + offset, values.data(), payload);
                auto view = V::from_bytes(bytes, std::span(*bytes).subspan(offset), values.bit_size(), width);
                EXPECT_EQ(view.empty(), size == 0);
                ASSERT_EQ(view.end() - view.begin(), ptrdiff_t(size));
                EXPECT_TRUE(std::equal(view.begin(), view.end(), values.begin()));
                for (uint64_t bit = 0; bit < values.bit_size(); ++bit)
                    for (uint8_t length = 1; length <= std::min<uint64_t>(64, values.bit_size() - bit); ++length)
                        ASSERT_EQ(values.get_int(bit, length), view.get_int(bit, length));
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
    return RUN_ALL_TESTS();
}
