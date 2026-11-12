// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
/*!\file shared_mapper.hpp
 * \brief shared_mapper.hpp contains shared_mapper_ifstream, an input file stream that also maps the file.
 */
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
//
// The views are only as immutable as the file. The mapping is shared
// (MAP_SHARED on POSIX) and lives as long as any view of it, also after the
// stream is closed or the file is removed. Until then the file must not be
// changed in place or truncated: views see writes to it, and reading a view
// whose pages were truncated away raises SIGBUS. Replace the file by renaming
// a new one over it instead; existing views keep the old contents.
//
// The stream and the mapping open the file by name separately. If the file is
// replaced in between, they refer to different files. Loading then still stays
// within the mapping (or throws), but may combine headers read from one file
// with payloads from the other.
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
