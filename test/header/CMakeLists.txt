# Modified version from Bio-Formats
# https://github.com/openmicroscopy/bioformats/blob/d3bb33eeda23e81f78fd25f658bfc14a4363805f/cpp/cmake/HeaderTest.cmake#L81-L113
# and SeqAn3
# https://github.com/seqan/seqan3/blob/c9719852dc2d42cd371bd5c65d3af93935f29f39/test/header/CMakeLists.txt
add_gtest()
add_library (sdsl_test_header INTERFACE)
set (header_all_target "sdsl_header_test")

file(GLOB SDSL_HEADER_ALL ${CMAKE_CURRENT_BINARY_DIR}/../../include/sdsl/*.hpp)

file (WRITE "${CMAKE_CURRENT_BINARY_DIR}/${header_all_target}.cpp" "int main() {}")
add_executable (${header_all_target} ${CMAKE_CURRENT_BINARY_DIR}/${header_all_target}.cpp)
target_link_libraries (${header_all_target} "gtest_main" "gtest")
target_link_libraries (${header_all_target} sdsl_test_header)
add_test (NAME "header/${header_all_target}" COMMAND ${header_all_target})

include_directories(${CMAKE_CURRENT_BINARY_DIR}/../../include
                    ${gtest_SOURCE_DIR}/include
)

foreach (header ${SDSL_HEADER_ALL})
    foreach (repeat 1 2)
        get_filename_component (header_target_name "${header}" NAME_WE)
        set (header_target_name "${header_target_name}-${repeat}")
        set (header_target_source "${CMAKE_CURRENT_BINARY_DIR}/${header_target_name}.cpp")
        set (header_target "${header_all_target}--${header_target_name}")

        string (REPLACE "-" "__" header_test_name_safe "${header_all_target}, ${header_target}")
        file (WRITE "${header_target_source}" "
#include <${header}>
#include <${header}>
#include <gtest/gtest.h>
TEST(${header_test_name_safe}) {}")
        add_library (${header_target} OBJECT "${header_target_source}")
        link_libraries(gtest)
        if (CMAKE_VERSION VERSION_LESS 3.12)
            target_compile_options (${header_target} PRIVATE $<TARGET_PROPERTY:sdsl_test_header,INTERFACE_COMPILE_OPTIONS>)
            target_compile_definitions (${header_target} PRIVATE $<TARGET_PROPERTY:sdsl_test_header,INTERFACE_COMPILE_DEFINITIONS>)
            target_include_directories (${header_target} PRIVATE $<TARGET_PROPERTY:sdsl_test_header,INTERFACE_INCLUDE_DIRECTORIES>)
            # target_link_libraries (${header_target} PRIVATE $<TARGET_PROPERTY:sdsl_test_header,INTERFACE_LINK_LIBRARIES>)
        else ()
            target_link_libraries (${header_target} sdsl_test_header)
        endif ()
        target_sources (${header_all_target} PRIVATE $<TARGET_OBJECTS:${header_target}>)
    endforeach ()
endforeach ()
