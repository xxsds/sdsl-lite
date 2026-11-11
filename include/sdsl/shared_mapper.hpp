#ifndef SDSL_SHARED_MAPPER_HPP
#define SDSL_SHARED_MAPPER_HPP
#include <fstream>
#include <memory>
#include <string>

#include <sdsl/int_vector_mapper.hpp>
#include <sdsl/util.hpp>

namespace sdsl
{
// An input file stream that also maps the whole file read-only.
// Loaders of types with view storage take their payloads from mapper()
// instead of copying them. Types with owned storage read the stream as usual.
class shared_mapper_ifstream : public std::ifstream
{
public:
    using byte_mapper = int_vector_mapper<8, std::ios_base::in>;

    // The stream is always opened in binary mode: loaders use its positions as
    // offsets into the mapping, which text mode would not preserve.
    explicit shared_mapper_ifstream(std::string const & filename, std::ios_base::openmode mode = std::ios_base::in) :
        std::ifstream(filename, mode | std::ios_base::binary)
    {
        // An empty file cannot be mapped and has no payload to map.
        if (good() && util::file_size(filename) > 0)
            m_mapper = std::make_shared<byte_mapper const>(filename, true);
    }

    //! The mapping of the whole file, or null if the file is empty or could not be opened.
    std::shared_ptr<byte_mapper const> const & mapper() const noexcept
    {
        return m_mapper;
    }

private:
    std::shared_ptr<byte_mapper const> m_mapper;
};
} // namespace sdsl
#endif
