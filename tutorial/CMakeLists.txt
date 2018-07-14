
file(GLOB tutorial_sources ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}/../include
)

foreach(tutorial_source ${tutorial_sources}) 
	get_filename_component(tutorial_name ${tutorial_source} NAME_WE)	
	add_executable(${tutorial_name} ${tutorial_name}.cpp)
endforeach(tutorial_source)
