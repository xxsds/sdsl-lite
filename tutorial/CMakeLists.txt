
file(GLOB tutorial_sources ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}/../include
	${CMAKE_CURRENT_BINARY_DIR}/../external/libdivsufsort/include
)

link_libraries(divsufsort divsufsort64)

foreach(tutorial_source ${tutorial_sources}) 
	get_filename_component(tutorial_name ${tutorial_source} NAME_WE)	
	add_executable(${tutorial_name} ${tutorial_name}.cpp)
endforeach(tutorial_source)
