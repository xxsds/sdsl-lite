cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")

## (1) Read and set library version 
cmake_policy(SET CMP0048 NEW)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" LIBRARY_VERSION_FULL)
string(REGEX REPLACE "[\n\r]" "" LIBRARY_VERSION_FULL "${LIBRARY_VERSION_FULL}")
string(REGEX REPLACE "^([0-9]+)\\.[0-9]+\\.[0-9]+$" "\\1" LIBRARY_VERSION_MAJOR "${LIBRARY_VERSION_FULL}")
string(REGEX REPLACE "^[0-9]+\\.([0-9]+)\\.[0-9]+$" "\\1" LIBRARY_VERSION_MINOR "${LIBRARY_VERSION_FULL}")
string(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+)$" "\\1" LIBRARY_VERSION_PATCH "${LIBRARY_VERSION_FULL}")

## (2) CREATE PROJECT 
project(sdsl VERSION ${LIBRARY_VERSION_FULL} LANGUAGES CXX C)

## (3) Project information ##
set(PROJECT_VENDOR "Simon Gog")
set(PROJECT_CONTACT "simon.gog@gmail.com")
set(PROJECT_URL "https://github.com/xxsds/sdsl-lite")
set(PROJECT_DESCRIPTION "SDSL: Succinct Data Structure Library")

## (4) Build information
set(CMAKE_BUILD_TYPE "Release")

## (5) Options
option(CODE_COVERAGE "Set ON to add code coverage compile options" OFF)
option(GENERATE_DOC "Set ON to genrate doxygen API reference in build/doc directory" OFF)

## (6) Compiler requirements
set (CMAKE_CXX_STANDARD 14)

## (7) Configure scripts
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/Make.helper.cmake"
		       "${CMAKE_CURRENT_SOURCE_DIR}/Make.helper" @ONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/lib/structure_tree.cpp.cmake"
               "${CMAKE_CURRENT_SOURCE_DIR}/lib/structure_tree.cpp" @ONLY)

find_package(Git)
if(GIT_FOUND)
    message("git found: ${GIT_EXECUTABLE}")
else()
    message(WARNING "git not found. Cloning of submodules will not work.")
endif()

add_subdirectory(external)
add_subdirectory(include)
add_subdirectory(lib)

## Add doxygen documentation generation target ##
find_package(Doxygen)
if(GENERATE_DOC AND DOXYGEN_FOUND)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/extras/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
add_custom_target(docs ALL
${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
COMMENT "Generating API documentation with Doxygen" VERBATIM
)
endif(GENERATE_DOC AND DOXYGEN_FOUND)

## Add 'uninstall' target ##
CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/CMakeModules/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)
ADD_CUSTOM_TARGET(uninstall-sdsl
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/CMakeModules/cmake_uninstall.cmake")

target_include_directories(sdsl PUBLIC include)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/sdsl-lite.pc.cmake"
               "${CMAKE_CURRENT_BINARY_DIR}/sdsl-lite.pc" @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/sdsl-lite.pc"
        DESTINATION "lib/pkgconfig")

add_subdirectory(test)
