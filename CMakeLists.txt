cmake_minimum_required (VERSION 3.13 FATAL_ERROR)
set (CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")

## (1) Read and set library version
include (ParseVersionFile)

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
option (CODE_COVERAGE "Set ON to add code coverage compile options" OFF)
option (SDSL_HEADER_TEST "Set ON to run header tests only" OFF)
option (GENERATE_DOC "Set ON to genrate doxygen API reference in build/doc directory" OFF)
option (USE_LIBCPP "Use the LLVM libc++ instead of GCC libstdc++ on OS X" ON)

## (6) Compiler requirements and compile options
include (CompilerFlags)

## (7) Configure scripts
configure_file ("${CMAKE_CURRENT_SOURCE_DIR}/Make.helper.cmake" "${CMAKE_CURRENT_SOURCE_DIR}/Make.helper" @ONLY)

enable_testing ()

# (8) main library target
set (gtest_dir ${CMAKE_CURRENT_LIST_DIR}/external/googletest)
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
add_subdirectory (test)

# (10) perform extra tasks such as doxygen / packaging and uninstall target
add_subdirectory (extras)

add_subdirectory (tutorial)
add_subdirectory (examples)
