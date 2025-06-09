function(find_xrandr)
    find_program(XRANDR_EXECUTABLE xrandr)

    if (NOT XRANDR_EXECUTABLE)
        message(FATAL_ERROR "xrandr could not be found. this program is only available for X11.")
    else()
        message(STATUS "found xrandr: ${XRANDR_EXECUTABLE}")
    endif()
endfunction()
