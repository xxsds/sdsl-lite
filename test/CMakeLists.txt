set (sdsl_test_targets "")
set (generated_files "")

## Create targets to downloads file specified in download.config
include (download_files.cmake)
add_gtest ()
## Generate targets for special inputs
function (generate_target_for_special_inputs tc_name abs_tc_name)
    set (int_vec_regex "int-vec\\.[0-9]+\\.[0-9]+\\..+")
    set (bit_vec_regex "bit-vec\\.*")
    set (int_vec_sa_regex "int-vec-sa\\.[0-9]+\\.[0-9]+\\..+")

    if ("${tc_name}" MATCHES "${int_vec_regex}" OR ${tc_name} MATCHES "${bit_vec_regex}")
        if ("${tc_name}" MATCHES "${int_vec_regex}")
            string (REPLACE "int-vec." "" tc_suf ${tc_name})
            string (REPLACE "." ";" tc_param ${tc_suf}) #  insert semicolons to get a list :)
        else ()
            string (REPLACE "bit-vec." "" tc_param ${tc_name})
        endif ()
        add_custom_command (OUTPUT ${abs_tc_name}
                            COMMAND $<TARGET_FILE:bit_vector_generator> ${abs_tc_name} ${tc_param}
                            DEPENDS bit_vector_generator
                            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                            COMMENT "Generating test case ${tc_name}."
                            VERBATIM)
        set (generated_files
             ${generated_files} ${abs_tc_name}
             PARENT_SCOPE)
    endif ()

    if ("${tc_name}" MATCHES "${int_vec_sa_regex}")
        string (REPLACE "int-vec-sa." "" tc_suf ${tc_name})
        string (REPLACE "." ";" tc_param ${tc_suf}) #  insert semicolons to get a list :)
        add_custom_command (OUTPUT ${abs_tc_name}
                            COMMAND $<TARGET_FILE:int_vector_generator> ${abs_tc_name} ${tc_param}
                            COMMAND $<TARGET_FILE:replace_int_vector_value> ${abs_tc_name} 0 1
                            DEPENDS int_vector_generator replace_int_vector_value
                            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                            COMMENT "Generating test case ${tc_name}."
                            VERBATIM)
        set (generated_files
             ${generated_files} ${abs_tc_name}
             PARENT_SCOPE)
    endif ()
endfunction (generate_target_for_special_inputs)

## Get all headers of the library, include the headers into compile_test.cpp.
## Compiling the created target will check for obvious errors.
file (GLOB SDSL_HEADER_ALL RELATIVE ${sdsl_include_directory} ${sdsl_include_directory}/sdsl/*.hpp)
string (REGEX REPLACE "([^;]+)[;]" "#include \<\\1\>\\n" SDSL_INCLUDE_ALL "${SDSL_HEADER_ALL};")
configure_file ("${CMAKE_CURRENT_SOURCE_DIR}/compile_test.cpp.cmake" "${CMAKE_CURRENT_SOURCE_DIR}/compile_test.cpp"
                @ONLY)

## Test sources are all files which ends with the suffix `_test.cpp`
## The following command will store all matching files in the variable `test_sources`
file (GLOB test_sources ${CMAKE_CURRENT_SOURCE_DIR}/*_test.cpp)

## Add required directionaries
include_directories (${sdsl_include_directory})
include_directories (SYSTEM ${gtest_dir}/googletest/include)

## Bogus warnings in GCC12
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU"
    AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 12
    AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 13)
    add_compile_options (-Wno-restrict)
endif ()

link_libraries ("gtest_main" "gtest")

if (SDSL_CEREAL)
    # Include via -isystem suppresses warnings of external dependencies in cereal
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem${CMAKE_CURRENT_SOURCE_DIR}/../external/cereal/include")
endif (SDSL_CEREAL)

add_executable (int_vector_generator int_vector_generator.cpp)
add_executable (bit_vector_generator bit_vector_generator.cpp)
add_executable (replace_int_vector_value replace_int_vector_value.cpp)

set (tmp_dirs "${CMAKE_CURRENT_LIST_DIR}/tmp/" "@/")
set (tmp_dirs_suffix "" "-im")

foreach (test_source ${test_sources})
    get_filename_component (test_name_we ${test_source} NAME_WE)

    string (REGEX REPLACE "_" "-" test_name_hy ${test_name_we})

    add_executable (${test_name_we} ${test_name_we}.cpp)
    list (APPEND sdsl_test_targets ${test_name_we})

    string (REGEX REPLACE "_[0-9]_" "_" config_name ${test_name_we})
    set (config_file ${CMAKE_CURRENT_SOURCE_DIR}/${config_name}.config)

    foreach (d RANGE 0 1)
        list (GET tmp_dirs ${d} tmp_dir)
        list (GET tmp_dirs_suffix ${d} tmp_dir_suffix)

        set (test_name "${test_name_hy}${tmp_dir_suffix}")

        # (1) Handle tests without .config file
        if (NOT EXISTS ${config_file})
            add_custom_target (${test_name}
                               COMMAND $<TARGET_FILE:${test_name_we}> ${tmp_dir}
                               DEPENDS test/${test_name_we} ${test_name_we}
                               WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                               COMMENT "Execute ${test_name}.")

            add_test (NAME ${test_name}
                      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                      COMMAND $<TARGET_FILE:${test_name_we}> ${tmp_dir})
            # (2) Handle tests with .config file
        else ()
            set (test_targets "")
            file (STRINGS ${config_file} config_lines REGEX "^[^#].+")
            set (config_line_cnt 0)
            foreach (config_line ${config_lines})
                math (EXPR config_line_cnt ${config_line_cnt}+1)
                # (2.1) Handle tests with one file per line in the .config file
                #       This is true if there is no semi-colon in the line (checked in the next line)
                if ("${config_line}" MATCHES "^[^;]+$")
                    get_filename_component (test_case_name ${config_line} NAME)
                    set (abs_test_case_name ${CMAKE_CURRENT_LIST_DIR}/test_cases/${test_case_name})

                    generate_target_for_special_inputs (${test_case_name} ${abs_test_case_name})

                    set (test_target ${test_name}_${test_case_name})
                    add_custom_target (${test_target}
                                       COMMAND $<TARGET_FILE:${test_name_we}> ${abs_test_case_name} ${tmp_dir}
                                       DEPENDS ${abs_test_case_name} ${test_name_we}
                                       WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                                       COMMENT "Execute ${test_name} on ${test_case_name}.")
                    add_test (NAME ${test_target}
                              WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                              COMMAND $<TARGET_FILE:${test_name_we}> ${abs_test_case_name} ${tmp_dir})
                    # (2.1) Handle tests with multiple files per line in the .config file
                else ()
                    #               Handle Cs[a|t]IntTest
                    if ("${test_name_we}" MATCHES "cs[a|t]_int_test")
                        list (GET config_line 0 test_case_name)
                        set (abs_test_case_name ${CMAKE_CURRENT_LIST_DIR}/test_cases/${test_case_name})

                        generate_target_for_special_inputs (${test_case_name} ${abs_test_case_name})

                        list (GET config_line 1 num_byte)
                        set (test_target ${test_name}_${test_case_name}_${num_byte})
                        add_custom_target (${test_target}
                                           COMMAND $<TARGET_FILE:${test_name_we}> ${abs_test_case_name} ${num_byte}
                                                   ${tmp_dir}
                                           DEPENDS ${abs_test_case_name} ${test_name_we}
                                           WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                                           COMMENT "Execute ${test_name} on ${test_case_name}.")

                        add_test (NAME ${test_target}
                                  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                                  COMMAND $<TARGET_FILE:${test_name_we}> ${abs_test_case_name} ${num_byte} ${tmp_dir})
                        list (APPEND test_targets ${test_target})
                        #               Handle K2TreapTest
                    elseif ("${test_name_we}" MATCHES "k2_treap_test")
                        set (concat_config "")
                        foreach (item ${config_line})
                            set (concat_config "${conact_config}${item}")
                        endforeach (item)
                        set (k2dim x y w)
                        list (GET config_line 0 test_case_name)
                        set (test_case_name "k2-${concat_config}")
                        if (NOT TARGET ${test_case_name})
                            set (abs_test_case_name ${CMAKE_CURRENT_LIST_DIR}/test_cases/${test_case_name})
                            set (abs_test_case_name_list "")
                            foreach (i 0 1 2)
                                list (GET k2dim ${i} dim)
                                set (local_abs_test_case_name ${abs_test_case_name}.${dim})
                                list (APPEND abs_test_case_name_list ${local_abs_test_case_name})
                                list (GET config_line ${i} test_case_suf)
                                string (REPLACE "." ";" test_case_param ${test_case_suf})
                                add_custom_command (OUTPUT ${local_abs_test_case_name}
                                                    COMMAND $<TARGET_FILE:int_vector_generator>
                                                            ${local_abs_test_case_name} ${test_case_param}
                                                    DEPENDS int_vector_generator
                                                    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                                                    COMMENT "Generating test case ${test_case_name}."
                                                    VERBATIM)
                                set (generated_files ${generated_files} ${local_abs_test_case_name})
                            endforeach (i)
                            add_custom_target (${test_case_name} DEPENDS ${abs_test_case_name_list})
                        endif ()
                        set (test_target ${test_name}_${test_case_name})
                        add_custom_target (${test_target}
                                           COMMAND $<TARGET_FILE:${test_name_we}> ${abs_test_case_name} ${tmp_dir}
                                           DEPENDS ${test_case_name} ${test_name_we}
                                           WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                                           COMMENT "Execute ${test_name} on ${test_case_name}.")
                        add_test (NAME ${test_target}
                                  COMMAND $<TARGET_FILE:${test_name_we}> ${abs_test_case_name} ${tmp_dir}
                                  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
                        list (APPEND test_targets ${test_target})
                    endif ()
                endif ()
            endforeach (config_line)

            add_custom_target (${test_name} DEPENDS ${test_targets})
        endif ()
    endforeach (d)
endforeach (test_source)

list (REMOVE_DUPLICATES generated_files)
add_custom_target (generate_test_files ALL DEPENDS ${generated_files})
add_custom_target (sdsl_test_targets DEPENDS ${sdsl_test_targets} ${generated_files})
add_custom_target (clean-test
                   COMMAND ${CMAKE_COMMAND} -E remove -f ${generated_files}
                   COMMENT "Remove generated test inputs.")
