set (sdsl_tutorials "")

file (GLOB tutorial_sources ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

include_directories (${CMAKE_CURRENT_SOURCE_DIR}/../include)

foreach (tutorial_source ${tutorial_sources})
    get_filename_component (tutorial_name ${tutorial_source} NAME_WE)
    add_executable (${tutorial_name} ${tutorial_name}.cpp)
    list (APPEND sdsl_tutorials ${tutorial_name})
endforeach (tutorial_source)

add_custom_target (sdsl_tutorials DEPENDS ${sdsl_tutorials})
