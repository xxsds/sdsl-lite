include (Colors)

# (1) generate docs
if (GENERATE_DOC)
    find_package (Doxygen)
    if (DOXYGEN_FOUND)
        configure_file (${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
        add_custom_target (
            docs ALL
            ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )
    endif (DOXYGEN_FOUND)
endif (GENERATE_DOC)

# (2) Add 'uninstall' target ##
configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/../CMakeModules/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/../CMakeModules/cmake_uninstall.cmake" IMMEDIATE @ONLY
)
add_custom_target (
    uninstall-sdsl "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/CMakeModules/cmake_uninstall.cmake"
)

# (3) packaging information
configure_file ("${CMAKE_CURRENT_SOURCE_DIR}/../sdsl-lite.pc.cmake" "${CMAKE_CURRENT_BINARY_DIR}/sdsl-lite.pc" @ONLY)
install (FILES "${CMAKE_CURRENT_BINARY_DIR}/sdsl-lite.pc" DESTINATION "lib/pkgconfig")

find_program (
    CLANG_FORMAT
    NAMES clang-format-4.0
          clang-format-3.9
          clang-format-3.8
          clang-format-3.7
          clang-format-3.6
          clang-format-3.5
          clang-format-3.4
          clang-format
)
if (CLANG_FORMAT)
    message (STATUS "${Green}clang-format found: ${CLANG_FORMAT}${ColourReset}")
else ()
    message (STATUS "${Yellow}clang-format NOT found!${ColourReset}")
endif (CLANG_FORMAT)

configure_file ("${CMAKE_CURRENT_SOURCE_DIR}/pre-commit.cmake" "${CMAKE_CURRENT_SOURCE_DIR}/pre-commit" @ONLY)
