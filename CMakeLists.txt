cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")

## (1) Read and set library version 
include(ParseVersionFile)

## (2) CREATE PROJECT 
cmake_policy(SET CMP0048 NEW) # no more project version warnings
project(sdsl VERSION ${LIBRARY_VERSION_FULL} LANGUAGES CXX C)

## (4) Build information
set(CMAKE_BUILD_TYPE "Release")

## (5) Options
option(CODE_COVERAGE "Set ON to add code coverage compile options" OFF)
option(GENERATE_DOC "Set ON to genrate doxygen API reference in build/doc directory" OFF)

## (6) Compiler requirements and compile options
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED on)
include(DetectCPUFeatures)
include(FindCxaDemangle)
CheckAndAppendCompilerFlags("Release" "-ffast-math")
CheckAndAppendCompilerFlags("Release" "-funroll-loops")

## (7) Configure scripts
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/Make.helper.cmake"
		       "${CMAKE_CURRENT_SOURCE_DIR}/Make.helper" @ONLY)

add_subdirectory(external)
add_subdirectory(include/sdsl)
add_subdirectory(lib)

# (8) main library target
target_include_directories(sdsl PUBLIC include)

# (9) test targets 
add_subdirectory(test)

# (10) perform extra tasks such as doxygen / packaging and uninstall target 
add_subdirectory(extras)
