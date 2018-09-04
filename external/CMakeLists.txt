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

## Add cereal
if(SDSL_CEREAL)
	MESSAGE(STATUS "Adding cereal")
	set(cereal_dir ${CMAKE_CURRENT_LIST_DIR}/cereal/include/cereal)
	set(cereal_file ${cereal_dir}/cereal.hpp)
	if(NOT EXISTS ${cereal_file})
	    EXECUTE_PROCESS(COMMAND git submodule update --init external/cereal
	                    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/..
	                    OUTPUT_QUIET
	            )
	endif(NOT EXISTS ${cereal_file})

	file(GLOB_RECURSE hppFiles RELATIVE ${cereal_dir} "${cereal_dir}/*.(h|hpp)") # select all .hpp-files

	foreach(hppFile ${hppFiles}) # copy each file
		configure_file( "${cereal_dir}/${hppFile}" "${CMAKE_CURRENT_BINARY_DIR}/cereal/include/cereal/${hppFile}" COPYONLY )
		install(FILES "${CMAKE_CURRENT_BINARY_DIR}/cereal/include/cereal/${hppFile}" DESTINATION external/cereal/include/cereal)
	endforeach(hppFile)
	MESSAGE(STATUS "Adding cereal - done")
endif(SDSL_CEREAL)
