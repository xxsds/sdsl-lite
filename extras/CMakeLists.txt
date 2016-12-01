
# (1) generate docs
if(GENERATE_DOC)
  find_package(Doxygen)
    if(DOXYGEN_FOUND) 
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/extras/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
        add_custom_target(docs ALL
        ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen" VERBATIM
        )
    endif(DOXYGEN_FOUND)
endif(GENERATE_DOC)

## (2) Add 'uninstall' target ##
CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/../CMakeModules/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/../CMakeModules/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)
ADD_CUSTOM_TARGET(uninstall-sdsl
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/CMakeModules/cmake_uninstall.cmake")

# (3) packaging information 
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../sdsl-lite.pc.cmake"
               "${CMAKE_CURRENT_BINARY_DIR}/../sdsl-lite.pc" @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/sdsl-lite.pc"
        DESTINATION "lib/pkgconfig")
