function(find_xrandr)
    find_program(XRANDR_EXECUTABLE xrandr)

    if (NOT XRANDR_EXECUTABLE)
        message(FATAL_ERROR "xrandr could not be found. try installing it before proceeding, also be advised that this program is intended to be run under X11.")
    else()
        message(STATUS "found xrandr: ${XRANDR_EXECUTABLE}")
    endif()
endfunction()
