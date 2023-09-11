find_package (Git)
if (NOT GIT_FOUND)
    message (WARNING "git not found. Cloning of submodules will not work.")
endif ()

# Modified version from SeqAn3
# https://github.com/seqan/seqan3/blob/fddd89708f8fa858521a9cf958900232c7e8ef89/test/cmake/seqan3_require_test.cmake
macro (add_gtest)
    set (SDSL_EXTERNAL_PROJECT_CMAKE_ARGS "")
    list (APPEND SDSL_EXTERNAL_PROJECT_CMAKE_ARGS "--no-warn-unused-cli")
    list (APPEND SDSL_EXTERNAL_PROJECT_CMAKE_ARGS "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}")
    list (APPEND SDSL_EXTERNAL_PROJECT_CMAKE_ARGS "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}")
    list (APPEND SDSL_EXTERNAL_PROJECT_CMAKE_ARGS "-DCMAKE_CXX_COMPILER_LAUNCHER=${CMAKE_CXX_COMPILER_LAUNCHER}")
    list (APPEND SDSL_EXTERNAL_PROJECT_CMAKE_ARGS "-DCMAKE_C_COMPILER_LAUNCHER=${CMAKE_C_COMPILER_LAUNCHER}")
    list (APPEND SDSL_EXTERNAL_PROJECT_CMAKE_ARGS "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
    list (APPEND SDSL_EXTERNAL_PROJECT_CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX=${PROJECT_BINARY_DIR}")
    list (APPEND SDSL_EXTERNAL_PROJECT_CMAKE_ARGS "-DCMAKE_VERBOSE_MAKEFILE=${CMAKE_VERBOSE_MAKEFILE}")

    if (NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/googletest/googletest/include/gtest/gtest.h")
        message (STATUS "Checking out googletest submodule")
        execute_process (
            COMMAND git submodule update --init external/googletest
            WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/..
            OUTPUT_QUIET
        )
    endif ()

    set (gtest_project_args ${SDSL_EXTERNAL_PROJECT_CMAKE_ARGS})
    list (APPEND gtest_project_args "-DCMAKE_CXX_FLAGS=-w")
    list (APPEND gtest_project_args "-DBUILD_GMOCK=0")

    # force that libraries are installed to `lib/`, because GNUInstallDirs might install it into `lib64/`
    list (APPEND gtest_project_args "-DCMAKE_INSTALL_LIBDIR=${PROJECT_BINARY_DIR}/lib/")

    set (gtest_main_path
         "${PROJECT_BINARY_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}gtest_main${CMAKE_STATIC_LIBRARY_SUFFIX}"
    )

    set (gtest_path "${PROJECT_BINARY_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}gtest${CMAKE_STATIC_LIBRARY_SUFFIX}")

    include (ExternalProject)
    ExternalProject_Add (
        gtest_project
        SOURCE_DIR "${gtest_dir}"
        CMAKE_ARGS "${gtest_project_args}"
        BUILD_BYPRODUCTS "${gtest_main_path}" "${gtest_path}"
    )
    unset (gtest_project_args)

    add_library (gtest_main STATIC IMPORTED)
    add_dependencies (gtest_main gtest_project)
    set_target_properties (gtest_main PROPERTIES IMPORTED_LOCATION "${gtest_main_path}")

    add_library (gtest STATIC IMPORTED)
    add_dependencies (gtest gtest_main)
    set_target_properties (gtest PROPERTIES IMPORTED_LOCATION "${gtest_path}")
    set_property (
        TARGET gtest
        APPEND
        PROPERTY INTERFACE_LINK_LIBRARIES "pthread"
    )

    unset (gtest_main_path)
    unset (gtest_path)

    add_custom_target (gtest_build DEPENDS gtest_main gtest)
endmacro ()

# Add cereal
if (SDSL_CEREAL)
    message (STATUS "Adding cereal")
    set (cereal_dir ${CMAKE_CURRENT_LIST_DIR}/cereal/include/cereal)
    set (cereal_file ${cereal_dir}/cereal.hpp)
    if (NOT EXISTS ${cereal_file})
        message (STATUS "  Checking out cereal submodule")
        execute_process (
            COMMAND git submodule update --init external/cereal
            WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/..
            OUTPUT_QUIET
        )
    endif (NOT EXISTS ${cereal_file})

    file (
        GLOB_RECURSE hppFiles
        RELATIVE ${cereal_dir}
        "${cereal_dir}/*.(h|hpp)"
    ) # select all .hpp-files

    message (STATUS "Adding cereal - done")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSDSL_HAS_CEREAL=1")
endif (SDSL_CEREAL)
