set(sdsl_tests "")
set(generated_files "")

## Create targets to downloads file specified in download.config
include(download_files.cmake)

## Get all headers of the library, include the headers into compile_test.cpp.
## Compiling the created target will check for obvious errors.
file(GLOB SDSL_HEADER_ALL ${CMAKE_CURRENT_BINARY_DIR}/../include/sdsl/*.hpp)
string(REGEX REPLACE "([^;]+)[;]" "#include \"\\1\"\\n" SDSL_INCLUDE_ALL "${SDSL_HEADER_ALL};")                                                        
string(REGEX REPLACE "${CMAKE_CURRENT_BINARY_DIR}/\\.\\./include/" "" SDSL_INCLUDE_ALL "${SDSL_INCLUDE_ALL}")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/compile_test.cpp.cmake"
               "${CMAKE_CURRENT_SOURCE_DIR}/compile_test.cpp" @ONLY)

## Test sources are all files which ends with the suffix `_test.cpp`
## The following command will store all matching files in the variable `test_sources`
file(GLOB test_sources ${CMAKE_CURRENT_SOURCE_DIR}/*_test.cpp)

## Add required directionaries
include_directories(${CMAKE_CURRENT_BINARY_DIR}/../include
                    ${CMAKE_CURRENT_BINARY_DIR}/../external/libdivsufsort/include
                    ${gtest_SOURCE_DIR}/include
)

link_libraries(sdsl gtest divsufsort divsufsort64)

add_executable(int_vector_generator EXCLUDE_FROM_ALL int_vector_generator.cpp)
add_executable(bit_vector_generator EXCLUDE_FROM_ALL bit_vector_generator.cpp)
add_executable(replace_int_vector_value EXCLUDE_FROM_ALL replace_int_vector_value.cpp)

set(tmp_dirs "${CMAKE_CURRENT_LIST_DIR}/tmp" "@")
set(tmp_dirs_suffix "" "-im")

foreach(test_source ${test_sources})
    get_filename_component(test_name_we ${test_source} NAME_WE)

    string(REGEX REPLACE "_" "-" test_name_hy ${test_name_we})

    add_executable(${test_name_we} EXCLUDE_FROM_ALL ${test_name_we}.cpp)

    set(config_file ${CMAKE_CURRENT_SOURCE_DIR}/${test_name_we}.config)

    foreach(d RANGE 0 1)
        list(GET tmp_dirs ${d} tmp_dir)
        list(GET tmp_dirs_suffix ${d} tmp_dir_suffix)
        message("HAHA ${d} ${tmp_dir} ${tmp_dir_suffix}")

        set(test_name "${test_name_hy}${tmp_dir_suffix}")

# (1) Handle tests without .config file
        if(NOT EXISTS ${config_file})
            ADD_CUSTOM_TARGET(${test_name}
                COMMAND $<TARGET_FILE:${test_name_we}> ${tmp_dir}
                DEPENDS test/${test_name_we} ${test_name_we}
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Execute ${test_name}."
            )
            list(APPEND sdsl_tests ${test_name})
# (2) Handle tests with .config file        
        else()
            set(test_targets "")
            file(STRINGS ${config_file} config_lines REGEX "^[^#].+")
            set(config_line_cnt 0)
            foreach(config_line ${config_lines})
                math(EXPR config_line_cnt ${config_line_cnt}+1)
# (2.1) Handle tests with one file per line in the .config file 
#       This is true if there is no semi-colon in the line (checked in the next line)
                if("${config_line}" MATCHES "^[^;]+$")
                    get_filename_component(test_case_name ${config_line} NAME)
                    set(abs_test_case_name ${CMAKE_CURRENT_LIST_DIR}/test_cases/${test_case_name})
                   
                    set(int_vec_regex "int-vec\\.[0-9]+\\.[0-9]+\\..+")
                    set(bit_vec_regex "bit-vec\\.*")
                    if("${test_case_name}" MATCHES "${int_vec_regex}")
                        string(REPLACE "int-vec." "" test_case_suf ${test_case_name})
                        #  insert semicolons to get a list :)
                        string(REPLACE "." ";" test_case_param ${test_case_suf}) 

                        add_custom_command(OUTPUT ${abs_test_case_name}
                                           COMMAND $<TARGET_FILE:int_vector_generator> ${abs_test_case_name}  ${test_case_param} 
                                           DEPENDS int_vector_generator
                                           WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                                           COMMENT "Generating test case ${test_case_name}."
                                           VERBATIM
                        )
                        list(APPEND generated_files ${abs_test_case_name})
                    elseif("${test_case_name}" MATCHES "${bit_vec_regex}")   
                        string(REPLACE "bit-vec." "" test_case_param ${test_case_name})
                        add_custom_command(OUTPUT ${abs_test_case_name}
                                           COMMAND $<TARGET_FILE:bit_vector_generator> ${abs_test_case_name} ${test_case_param} 
                                           DEPENDS bit_vector_generator
                                           WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                                           COMMENT "Generating test case ${test_case_name}."
                                           VERBATIM
                        )
                        list(APPEND generated_files ${abs_test_case_name})
                    endif()

                    set(test_target ${test_name}_${test_case_name})
                    add_custom_target(${test_target}
                        COMMAND $<TARGET_FILE:${test_name_we}> ${abs_test_case_name} ${tmp_dir}/${test_name_we}_${test_case_name} ${tmp_dir}
                        DEPENDS ${abs_test_case_name} ${test_name_we}
                        WORKING_DIRECTORY ${CMAKE_BINARY_DIR} 
                        COMMENT "Execute ${test_name} on ${test_case_name}."
                    )
                    list(APPEND test_targets ${test_target})
# (2.1) Handle tests with multiple files per line in the .config file 
                else() 
#               Handle Cs[a|t]IntTest
                    if ("${test_name_we}" MATCHES "cs[a|t]_int_test") 
                        list(GET config_line 0 test_case_name)
                        set(abs_test_case_name ${CMAKE_CURRENT_LIST_DIR}/test_cases/${test_case_name})

                        set(int_vec_sa_regex "int-vec-sa\\.[0-9]+\\.[0-9]+\\..+")
                        if("${test_case_name}" MATCHES "${int_vec_sa_regex}")
                            string(REPLACE "int-vec-sa." "" test_case_suf ${test_case_name})
                            #  insert semicolons to get a list :)
                            string(REPLACE "." ";" test_case_param ${test_case_suf}) 

                            add_custom_command(OUTPUT ${abs_test_case_name}
                                               COMMAND $<TARGET_FILE:int_vector_generator> ${abs_test_case_name}  ${test_case_param} 
                                               COMMAND $<TARGET_FILE:replace_int_vector_value> ${abs_test_case_name} 0 1
                                               DEPENDS int_vector_generator replace_int_vector_value 
                                               WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                                               COMMENT "Generating test case ${test_case_name}."
                                               VERBATIM
                            )
                            list(APPEND generated_files ${abs_test_case_name})
                        endif()


                        list(GET config_line 1 num_byte)
                        set(test_target ${test_name}_${test_case_name}_${num_byte})
                        add_custom_target(${test_target}
                            COMMAND $<TARGET_FILE:${test_name_we}> ${abs_test_case_name} ${num_byte} ${tmp_dir}/${test_name_we}_${tase_case_name}_${num_byte} ${tmp_dir}
                            DEPENDS ${abs_test_case_name} ${test_name_we}
                            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                            COMMENT "Execute ${test_name} on ${test_case_name}."        
                        )
                        list(APPEND test_targets ${test_target})
#               Handle K2TreapTest    
                    elseif("${test_name_we}" MATCHES "k2_treap_test")
                        set(concat_config "")
#                    STRING(CONCAT concat_config ${config_line})
                        foreach(item ${config_line})
                            set(concat_config "${conact_config}${item}")
                        endforeach(item)
                        set(k2dim x y w)
                        list(GET config_line 0 test_case_name)
                        set(test_case_name "k2-${concat_config}")
                        if(TARGET ${test_case_name})
                        else()
                            set(abs_test_case_name ${CMAKE_CURRENT_LIST_DIR}/test_cases/${test_case_name})
                            set(abs_test_case_name_list "")
                            foreach(i 0 1 2)
                                list(GET k2dim ${i} dim)
                                set(local_abs_test_case_name ${abs_test_case_name}.${dim})
                                list(APPEND abs_test_case_name_list ${local_abs_test_case_name})
                                list(GET config_line ${i} test_case_suf)
                                string(REPLACE "." ";" test_case_param ${test_case_suf})
                                add_custom_command(OUTPUT ${local_abs_test_case_name}
                                        COMMAND $<TARGET_FILE:int_vector_generator> ${local_abs_test_case_name} ${test_case_param}
                                        DEPENDS int_vector_generator
                                        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                                        COMMENT "Generating test case ${test_case_name}."
                                        VERBATIM        
                                )
                            endforeach(i)
                            add_custom_target(${test_case_name} 
                                    DEPENDS ${abs_test_case_name_list})
                        endif()
                        set(test_target ${test_name}_${test_case_name})
                        add_custom_target(${test_target}
                                    COMMAND $<TARGET_FILE:${test_name_we}> ${abs_test_case_name} ${tmp_dir}/${test_name_we}_${test_case_name}
                                    DEPENDS ${test_case_name} ${test_name_we}
                                    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                                    COMMENT "Execute ${test_name} on ${test_case_name}."
                                )
                        list(APPEND test_targets ${test_target})
                    endif()
                endif()
            endforeach(config_line)

            add_custom_target(${test_name} DEPENDS ${test_targets}) 
            list(APPEND sdsl_tests ${test_name})
        endif()
    endforeach(d)
endforeach(test_source)

add_custom_target(test-sdsl DEPENDS ${sdsl_tests})

add_custom_target(clean-test
                 COMMAND ${CMAKE_COMMAND} -E remove -f ${generated_files}
                 COMMENT "Remove generated and downloaded test inputs.")


# TODO: special handling for in-memory test
