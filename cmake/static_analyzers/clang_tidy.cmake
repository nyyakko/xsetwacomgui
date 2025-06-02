function(enable_clang_tidy PROJECT)
    find_program(CLANGTIDY clang-tidy)

    if (NOT CLANGTIDY)
        message(WARNING "[${PROJECT}] Couldn't find a valid ``clang-tidy`` installation.")
        return()
    endif()

    if (NOT CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
        get_target_property(TARGET_PCH ${PROJECT} INTERFACE_PRECOMPILE_HEADERS)

        if("${TARGET_PCH}" STREQUAL "TARGET_PCH-NOTFOUND")
            get_target_property(TARGET_PCH ${PROJECT} PRECOMPILE_HEADERS)
        endif()

        if(NOT ("${TARGET_PCH}" STREQUAL "TARGET_PCH-NOTFOUND"))
            message(SEND_ERROR "clang-tidy cannot be enabled with non-clang compiler and PCH, clang-tidy fails to handle gcc's PCH file")
        endif()
    endif()

    set(CLANG_TIDY_OPTIONS ${CLANGTIDY}
        --extra-arg=-Wno-unknown-warning-option
        --extra-arg=-Wno-ignored-optimization-argument
        --extra-arg=-Wno-unused-command-line-argument
        -warnings-as-errors=*
        --use-color
        --p)

    if("${CLANG_TIDY_OPTIONS_DRIVER_MODE}" STREQUAL "cl")
        set(CLANG_TIDY_OPTIONS ${CLANG_TIDY_OPTIONS} -extra-arg=/std:c++23)
    else()
        set(CLANG_TIDY_OPTIONS ${CLANG_TIDY_OPTIONS} -extra-arg=-std=c++23)
    endif()

    set_target_properties(${PROJECT} PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_OPTIONS}")
endfunction()
