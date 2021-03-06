include(CheckIncludeFile)
include(CheckIncludeFileCXX)
include(CheckTypeSize)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/structure_tree.hpp.cmake"
               "${CMAKE_CURRENT_SOURCE_DIR}/structure_tree.hpp" @ONLY)

## Check for header files ##
check_include_file_CXX(cstdio HAVE_STDIO)
