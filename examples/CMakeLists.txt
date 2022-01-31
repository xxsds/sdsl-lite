set(sdsl_examples "")

file(GLOB example_sources ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

include_directories (${sdsl_include_directory})

foreach(example_source ${example_sources})
	get_filename_component(example_name ${example_source} NAME_WE)
	add_executable(${example_name} EXCLUDE_FROM_ALL ${example_name}.cpp)
	list(APPEND sdsl_examples ${example_name})
endforeach(example_source)

add_custom_target(sdsl_examples DEPENDS ${sdsl_examples})
