#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <sdsl/construct.hpp>
#include <sdsl/csa_sada.hpp>

#include <gtest/gtest.h>

namespace
{

using csa_type = sdsl::csa_sada<>;

//! Builds a CSA and returns its serialized form, which is what load_in_place reads.
std::vector<char> serialize_csa(csa_type & csa)
{
    std::string text = "abracadabra barbara bark";
    sdsl::construct_im(csa, text, 1);
    std::stringstream stream;
    csa.serialize(stream);
    std::string const serialized = stream.str();
    // A heap buffer, so the uint64_t members the CSA points at are aligned as they are in a file.
    return std::vector<char>(serialized.begin(), serialized.end());
}

TEST(load_in_place, borrows_a_csa_equal_to_the_one_serialized)
{
    csa_type built;
    std::vector<char> const buffer = serialize_csa(built);

    csa_type borrowed;
    char const * addr = buffer.data();
    borrowed.load_in_place(addr, buffer.data() + buffer.size());

    ASSERT_EQ(built.size(), borrowed.size());
    for (size_t i = 0; i < built.size(); ++i)
    {
        ASSERT_EQ(built[i], borrowed[i]) << "suffix array values differ at " << i;
    }
    // The structure describes itself completely, so reading it consumes the buffer exactly.
    ASSERT_EQ(addr, buffer.data() + buffer.size());
}

TEST(load_in_place, throws_on_a_buffer_that_ends_inside_the_structure)
{
    csa_type built;
    std::vector<char> const buffer = serialize_csa(built);

    for (size_t length : {size_t{0}, size_t{8}, buffer.size() / 2, buffer.size() - 1})
    {
        csa_type borrowed;
        char const * addr = buffer.data();
        ASSERT_THROW(borrowed.load_in_place(addr, buffer.data() + length), std::out_of_range)
            << "truncated to " << length << " of " << buffer.size() << " bytes";
    }
}

} // namespace
