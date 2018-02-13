// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.
/*! \file platform.hpp
    \brief util.hpp contains platform dependend macros.
    \author Christopher Pockrandt
*/
#ifndef INCLUDED_SDSL_PLATFORM
#define INCLUDED_SDSL_PLATFORM

//! Namespace for the succinct data structure library.
namespace sdsl {

#if defined(__clang__)
#define COMPILER_CLANG
#endif

#if defined(__GNUC__) && !defined(COMPILER_CLANG)
#define COMPILER_GCC
#endif

// eliminate fallthrough warnings
#define SDSL_FALLTHROUGH
#if defined(__has_cpp_attribute)
    #if __has_cpp_attribute(fallthrough)
        #undef SDSL_FALLTHROUGH
        #if __cplusplus < 201500 && defined(COMPILER_GCC)
            #define SDSL_FALLTHROUGH [[gnu::fallthrough]];
        #elif __cplusplus < 201500 && defined(COMPILER_CLANG)
            #define SDSL_FALLTHROUGH [[clang::fallthrough]];
        #else
            #define SDSL_FALLTHROUGH [[fallthrough]];
        #endif
    #endif
#endif

} // end namespace sdsl

#endif
