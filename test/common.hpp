#ifndef INCLUDED_SDSL_TEST_COMMON
#define INCLUDED_SDSL_TEST_COMMON

#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

#include <sdsl/cereal.hpp>
#include <sdsl/io.hpp>

// gtest only finds this operator if it is declared before gtest is included.
template <class T>
std::ostream & operator<<(std::ostream & os, std::pair<T, T> const & v)
{
    os << "[" << v.first << ", " << v.second << "]";
    return os;
}

#include <gtest/gtest.h>

int init_2_arg_test(int argc,
                    char ** argv,
                    std::string index_type,
                    std::string & test_file,
                    std::string & temp_dir,
                    std::string & temp_file)
{
    if (argc < 3)
    {
        // LCOV_EXCL_START
        std::cout << "Usage: " << argv[0] << " test_file tmp_dir\n\
        (1) Generates an index out of test_file; stores it in a temporary file.\n\
        (2) Performs tests.\n\
        (3) Deletes the temporary file."
                  << std::endl;
        std::cout << "Only " << argc << " parameters provided" << std::endl;
        return 1;
        // LCOV_EXCL_STOP
    }
    test_file = argv[1];
    temp_dir = argv[2];
    temp_file = sdsl::tmp_file(temp_dir + "/" + index_type, index_type);
    return 0;
}

int init_3_arg_test(int argc,
                    char ** argv,
                    std::string index_type,
                    std::string & test_file,
                    uint8_t & num_bytes,
                    std::string & temp_dir,
                    std::string & temp_file)
{
    if (argc < 4)
    {
        // LCOV_EXCL_START
        std::cout << "Usage: " << argv[0] << " test_file num_bytes tmp_dir\n\
        (1) Generates an index out of test_file interpreted as a sequence of\n\
            symbols each consisting of num_bytes bytes;\n\
            stores it in a temporary file.\n\
        (2) Performs tests.\n\
        (3) Deletes the temporary file."
                  << std::endl;
        return 1;
        // LCOV_EXCL_STOP
    }
    test_file = argv[1];
    num_bytes = atoi(argv[2]);
    temp_dir = argv[3];
    temp_file = sdsl::tmp_file(temp_dir + "/" + index_type + "_" + argv[2], index_type);
    return 0;
}

#if SDSL_HAS_CEREAL
// Largest structure, in bytes (sdsl::size_in_bytes), that test_cereal_archives also round-trips through the JSON and
// XML archives. Text archives store an int_vector element by element, a bit_vector bit by bit: for the largest test
// inputs that writes about 1 GB per archive and takes tens of seconds, whereas the binary archives take milliseconds.
// Larger structures are only round-tripped through the binary archives.
inline constexpr uint64_t cereal_text_archive_max_bytes = uint64_t{1} << 20;

// Saves `object` with `out_archive_t`, loads it into a new object with `in_archive_t` and expects both to be equal.
// If `vector_t` is not void, `vector` is set as the supported bit vector of the loaded object before the comparison.
template <typename in_archive_t, typename out_archive_t, typename t, typename vector_t>
void do_serialisation(t const & object, vector_t const * vector)
{
    std::stringstream buffer;
    {
        out_archive_t oarchive{buffer};
        oarchive(object);
    }

    t loaded{};
    {
        in_archive_t iarchive{buffer};
        iarchive(loaded);
    }
    if constexpr (!std::is_void_v<vector_t>)
        loaded.set_vector(vector);
    EXPECT_EQ(object, loaded);
}

// Round-trips `object` through the binary, portable binary, JSON and XML archives. Skips the current test after the
// binary archives if `object` exceeds cereal_text_archive_max_bytes.
// For a rank or select support, `vector` is the supported bit vector; it is set on each loaded object.
template <typename t, typename vector_t = void>
void test_cereal_archives(t const & object, vector_t const * vector = nullptr)
{
    do_serialisation<cereal::BinaryInputArchive, cereal::BinaryOutputArchive>(object, vector);
    do_serialisation<cereal::PortableBinaryInputArchive, cereal::PortableBinaryOutputArchive>(object, vector);

    if (uint64_t const bytes = sdsl::size_in_bytes(object); bytes > cereal_text_archive_max_bytes)
        GTEST_SKIP() << "Skipping the JSON and XML archives for " << bytes << " bytes";

    do_serialisation<cereal::JSONInputArchive, cereal::JSONOutputArchive>(object, vector);
    do_serialisation<cereal::XMLInputArchive, cereal::XMLOutputArchive>(object, vector);
}

// Loads a `t` from `file`, which was written by sdsl::store_to_file, and passes it to test_cereal_archives.
template <typename t>
void test_cereal_archives_from_file(std::string const & file)
{
    t object;
    ASSERT_TRUE(sdsl::load_from_file(object, file));
    test_cereal_archives(object);
}
#endif // SDSL_HAS_CEREAL

#endif
