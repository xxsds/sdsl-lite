include(CheckCXXCompilerFlag)

macro(AppendCompilerFlags)
  cmake_parse_arguments(ARG "CONFIG" "" ${ARGN})
  # determine to which flags to add
  set(CXX_FLAGS CMAKE_CXX_FLAGS)
  if(NOT "${ARG_CONFIG}" STREQUAL "")
      string(TOUPPER ${ARG_CONFIG} ARG_CONFIG)
      set(CXX_FLAGS CMAKE_CXX_FLAGS_${ARG_CONFIG})
  endif()
  
  # add all the passed flags
  foreach(flag ${ARG_UNPARSED_ARGUMENTS})
    check_cxx_compiler_flag( "${flag}"   COMPILER_SUPPORTS_${flag} )
    if(COMPILER_SUPPORTS_${flag})
      set(${CXX_FLAGS} "${${CXX_FLAGS}} ${flag}")
    endif()
  endforeach()
endmacro(AppendCompilerFlags)
