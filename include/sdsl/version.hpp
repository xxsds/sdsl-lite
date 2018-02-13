// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
/*! \file version.hpp
    \brief version.hpp contains version numbers of the release.
    \author Christopher Pockrandt
*/
#ifndef INCLUDED_SDSL_VERSION
#define INCLUDED_SDSL_VERSION

#include <string>

//!\brief The major version as MACRO.
#define SDSL_VERSION_MAJOR 3
//!\brief The minor version as MACRO.
#define SDSL_VERSION_MINOR 0
//!\brief The patch version as MACRO.
#define SDSL_VERSION_PATCH 0

//!\brief The full version as MACRO (number).
#define SDSL_VERSION (SDSL_VERSION_MAJOR * 10000 \
                     + SDSL_VERSION_MINOR * 100 \
                     + SDSL_VERSION_PATCH)

namespace sdsl
{

//!\brief The major version.
constexpr uint8_t sdsl_version_major = SDSL_VERSION_MAJOR;
//!\brief The minor version.
constexpr uint8_t sdsl_version_minor = SDSL_VERSION_MINOR;
//!\brief The patch version.
constexpr uint8_t sdsl_version_patch = SDSL_VERSION_PATCH;

//!\brief The full version as `std::string`.
std::string const sdsl_version = std::to_string(sdsl_version_major) + "." +
                                 std::to_string(sdsl_version_minor) + "." +
                                 std::to_string(sdsl_version_patch);


} // end namespace

#endif
