include (CheckCXXCompilerFlag)

macro (CheckAndAppendCompilerFlags CONFIG flags)
    # determine to which flags to add
    set (CXX_FLAGS CMAKE_CXX_FLAGS)
    if (NOT "${CONFIG}" STREQUAL "")
        string (TOUPPER ${CONFIG} UPCONFIG)
        set (CXX_FLAGS CMAKE_CXX_FLAGS_${UPCONFIG})
    endif ()

    # add all the passed flags
    foreach (flag ${flags})
        string (TOUPPER ${flag} FLAG)
        check_cxx_compiler_flag ("${flag}" CHECK_RESULT)
        if (CHECK_RESULT)
            set (${CXX_FLAGS} "${${CXX_FLAGS}} ${flag}")
        endif ()
    endforeach ()
endmacro (CheckAndAppendCompilerFlags)
