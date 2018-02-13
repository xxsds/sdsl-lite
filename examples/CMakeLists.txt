
file(GLOB example_sources ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}/../include
	${CMAKE_CURRENT_BINARY_DIR}/../external/libdivsufsort/include
)

link_libraries(divsufsort divsufsort64)

foreach(example_source ${example_sources}) 
	get_filename_component(example_name ${example_source} NAME_WE)	
	add_executable(${example_name} EXCLUDE_FROM_ALL ${example_name}.cpp)
endforeach(example_source)
