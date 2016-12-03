## download 
## Download files specified in a ${CMAKE_CURRENT_LIST_DIR}/download.config
#  Each file should be specified on a seperate line as tuple (file;URL)

file(STRINGS ${CMAKE_CURRENT_LIST_DIR}/download.config download_config_lines REGEX "^[^#].*")
foreach(download_config_line ${download_config_lines})
    list(GET download_config_line 0 download_file)
    list(GET download_config_line 1 download_url)
    string(REPLACE ".tar.gz" "" download_file_we ${download_file})
    set(abs_test_case_name ${CMAKE_CURRENT_LIST_DIR}/test_cases/${download_file_we})
    
    add_custom_command(OUTPUT ${abs_test_case_name}
                      COMMAND ${CMAKE_COMMAND} -Ddownload_url=${download_url} -Dtest_case_dir=${CMAKE_CURRENT_LIST_DIR}/test_cases -P ${CMAKE_CURRENT_LIST_DIR}/download.cmake
                      WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
                      COMMENT "Checking or downloading test case ${download_file}."
                      VERBATIM
    )
    add_custom_target(${download_file_we}
                      DEPENDS ${abs_test_case_name})
    list(APPEND generated_files ${abs_test_case_name})
endforeach(download_config_line)


