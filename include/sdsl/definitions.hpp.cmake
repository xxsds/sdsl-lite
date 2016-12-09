// Copyright (c) 2016, the SDSL Project Authors.  All rights reserved.
// Please see the AUTHORS file for details.  Use of this source code is governed
// by a BSD license that can be found in the LICENSE file.

#ifndef SDSL_DEFINITIONS_HPP
#define SDSL_DEFINITIONS_HPP

#include <string>

#if @HAVE_SSE42@
#define HAVE_SSE42 1
#endif

#if @HAVE_MODETI@
#define HAVE_MODETI
#endif

#if @HAVE_CXA_DEMANGLE@
#define HAVE_DEMANGLE
#endif

#define SDSL_PROJECT_DIR "@PROJECT_SOURCE_DIR@"

#if @MSVC_COMPILER@
#define MSVC_COMPILER
#endif

#endif