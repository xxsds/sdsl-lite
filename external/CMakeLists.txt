find_package(Git)
if(NOT GIT_FOUND)   
    message(WARNING "git not found. Cloning of submodules will not work.")
endif()

## Add gtest 
set(gtest_dir ${CMAKE_CURRENT_LIST_DIR}/googletest)
set(gtest_file ${gtest_dir}/CMakeLists.txt)
if(NOT EXISTS ${gtest_file})
    EXECUTE_PROCESS(COMMAND git submodule update --init external/googletest
                    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/..
                    OUTPUT_QUIET
            )
endif(NOT EXISTS ${gtest_file})

add_subdirectory(googletest EXCLUDE_FROM_ALL)

   
