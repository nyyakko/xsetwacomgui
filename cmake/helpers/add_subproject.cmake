function(add_subproject)
    cmake_parse_arguments(
        PARSED
        ""
        ""
        "CC_OPTIONS;LD_OPTIONS;LIBRARIES"
        ${ARGN}
    )

    set(${ARGV0}_CompilerOptions ${${PROJECT_NAME}_CompilerOptions})
    if(PARSED_CC_OPTIONS)
        list(APPEND ${ARGV0}_CompilerOptions "${PARSED_CC_OPTIONS}")
    endif()

    set(${ARGV0}_LinkerOptions "${${PROJECT_NAME}_LinkerOptions}")
    if(PARSED_LD_OPTIONS)
        list(APPEND ${ARGV0}_LinkerOptions "${PARSED_LD_OPTIONS}")
    endif()

    if(PARSED_LIBRARIES)
        set(${ARGV0}_ExternalLibraries ${PARSED_LIBRARIES})
    endif()

    add_subdirectory(${ARGV0})
endfunction()
