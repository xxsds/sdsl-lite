# TODO: move to CMakeLists.txt? or is it needed by examples/tutorials?
# TODO: should the files be made a target?

# TODO: path correct?
file (GLOB ZIPPED_TEST_CASES ${CMAKE_CURRENT_LIST_DIR}/test_cases/*.tar.gz)

file (MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test_cases")

foreach (zipped_test_case ${ZIPPED_TEST_CASES})
    get_filename_component (file_name ${zipped_test_case} NAME)
    string (REPLACE ".tar.gz" "" file_name ${file_name})
    if (NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_cases/${file_name}")
        message ("Unzipped ${file_name}")
        execute_process (COMMAND ${CMAKE_COMMAND} -E tar xzf ${zipped_test_case}
                         WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/test_cases/")
    endif ()
endforeach (zipped_test_case)
