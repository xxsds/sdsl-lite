cmake_minimum_required (VERSION 3.13 FATAL_ERROR)
set (CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")

## (1) Read and set library version
include (ParseVersionFile)

# check if this is the main iproject
# This can be removed if cmake 3.21 is used
# https://cmake.org/cmake/help/latest/variable/PROJECT_IS_TOP_LEVEL.html
set(PROJECT_IS_TOP_LEVEL FALSE)
if (NOT DEFINED PROJECT_NAME)
    set(PROJECT_IS_TOP_LEVEL TRUE)
endif ()

## (2) CREATE PROJECT
cmake_policy (SET CMP0048 NEW) # no more project version warnings
project (sdsl
         VERSION ${LIBRARY_VERSION_FULL}
         LANGUAGES CXX C)
set (PROJECT_URL "https://github.com/xxsds/sdsl-lite")
set (PROJECT_DESCRIPTION "The Succinct Data Structure Library")

## (4) Build information. Default = Release
if (NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE "Release")
endif ()

## (5) Options
option (SDSL_CODE_COVERAGE "Set ON to add code coverage compile options" OFF)
option (SDSL_HEADER_TEST "Set ON to run header tests only" OFF)
option (SDSL_BUILD_TESTS "Set ON to build tests" ${PROJECT_IS_TOP_LEVEL})
option (SDSL_BUILD_TUTORIAL "Set ON to build tutorials" ${PROJECT_IS_TOP_LEVEL})
option (SDSL_BUILD_EXAMPLES "Set ON to build examples" ${PROJECT_IS_TOP_LEVEL})
option (SDSL_GENERATE_DOC "Set ON to genrate doxygen API reference in build/doc directory" OFF)
option (SDSL_USE_LIBCPP "Use the LLVM libc++ instead of GCC libstdc++ on OS X" ON)

## (6) Compiler requirements and compile options
include (CompilerFlags)

## (7) Configure scripts
configure_file ("${CMAKE_CURRENT_SOURCE_DIR}/Make.helper.cmake" "${CMAKE_CURRENT_SOURCE_DIR}/Make.helper" @ONLY)


if (SDSL_BUILD_TESTS)
    enable_testing ()

    # (8) main library target
    set (gtest_dir ${CMAKE_CURRENT_LIST_DIR}/external/googletest)
endif()

add_subdirectory (external)
add_subdirectory (lib)

set (sdsl_include_directory "${CMAKE_CURRENT_SOURCE_DIR}/include")
file (GLOB HEADERS "${sdsl_include_directory}/sdsl/*.hpp")
add_custom_target (sdsl SOURCES ${HEADERS})

# (9) test targets
if (SDSL_HEADER_TEST)
    add_subdirectory (test/header)
    return ()
endif ()
if (SDSL_BUILD_TESTS)
    add_subdirectory (test)
endif()

# (10) perform extra tasks such as doxygen / packaging and uninstall target
add_subdirectory (extras)

if (SDSL_BUILD_TUTORIAL)
    add_subdirectory (tutorial)
endif()
if (SDSL_BUILD_EXAMPLES)
    add_subdirectory (examples)
endif()
